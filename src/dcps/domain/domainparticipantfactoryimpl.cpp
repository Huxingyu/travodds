#include "dcps/domain/domainparticipantfactoryimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "qos/qospolicycheck.h"
#include "common/log/logger.h"
#include "rtps/transport/asiocontext.h"
#include "common/sys/sysinfo.h"
#ifdef DDSSECURITY
#include "security/propertyqospolicy.h"
#endif // DDSSECURITY
#include <random>
#ifdef _WIN32

#else
#include <dlfcn.h>
#endif

USING_TRAVODDS_NAMESPACE

DomainParticipantFactory* DomainParticipantFactory::get_instance()
{
    return DomainParticipantFactoryImpl::get_instance();
}

void DomainParticipantFactory::finalize()
{
	return DomainParticipantFactoryImpl::finalize();
}

DomainParticipantFactoryImpl::DomainParticipantFactoryImpl()
{

}

DomainParticipant* DomainParticipantFactoryImpl::create_participant(DomainId_t domain_id, const DomainParticipantQos& qos, DomainParticipantListener* listener, const StatusMask& mask)
{
    return create_participant(domain_id, qos, listener, mask, GUID_UNKNOWN);
}

DomainParticipant* DomainParticipantFactoryImpl::create_participant(DomainId_t domain_id, const DomainParticipantQos& qos, DomainParticipantListener* listener, const StatusMask& mask, const GUID_t& guid) 
{
    if (domain_id > DOMAIN_ID_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "DomainId_t is Err, should 0 <= domain_id <= 232 ");
		return nullptr;
    }
	const DomainParticipantQos& qosT = (&qos == &PARTICIPANT_QOS_DEFAULT) ? defaultParticipantQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}

    GUID_t guidt = guid;

    {
        std::lock_guard<std::mutex> lock(mtxParticipants_);
        if (guidt == GUID_UNKNOWN) {
            guidt.entityId = ENTITYID_PARTICIPANT;
            do {
                GenerateGuidPrefix(domain_id, guidt);
            } while (participants_.find(Conversion::ToInstanceHandle(guid)) != participants_.end());
        } else {
            guidt.entityId = ENTITYID_PARTICIPANT;
            if (participants_.find(Conversion::ToInstanceHandle(guidt)) != participants_.end()) {
                TRAVODDS_LOG(LOG_ERROR, "GUID is exist");
                return nullptr;
            }
        }
        participants_.insert({ Conversion::ToInstanceHandle(guidt), nullptr });
    }

	DomainParticipant* domainPart = nullptr;

#ifdef DDSSECURITY

	DDSSecurityPluginInstance* pluginPtr = NULL;
    Property_t pluginNameProperty;
    bool hasPluginConfig = false; // 是否配置了插件
    for (int index = 0; index < qos.property.value.size(); ++index)
    {
        Property_t tmpProperty = qos.property.value[index];
        // 如果有使用的插件名称属性
        if (tmpProperty.name == std::string("dds.sec.plugin_name"))
        {
            hasPluginConfig = true;
            pluginNameProperty = tmpProperty; //找到插件键值对对象，即使用的插件，比如BuiltinPlugin
            break;
        }
    }
    if (hasPluginConfig) {
        // 从安全插件数组secPlugins_中找到对应的插件实例
        auto it = secPlugins_.find(pluginNameProperty.value);
        if (it != secPlugins_.end()) {
            pluginPtr = &(it->second.inst);
            TRAVODDS_LOG(LOG_DEBUG, "Find plugin in secPlugins_");
        }
        else { //未找到此域参与者对应的安全插件模块,域参与者创建失败
            TRAVODDS_LOG(LOG_ERROR, "Not find plugin in secPlugins_");
            return nullptr;
        }
    }
    std::unique_ptr<DomainParticipantImpl> domainParticipant = std::make_unique<DomainParticipantImpl>(domain_id, qosT, listener, mask, guidt,pluginPtr);
#else
    std::unique_ptr<DomainParticipantImpl> domainParticipant = std::make_unique<DomainParticipantImpl>(domain_id, qosT, listener, mask，guidt);
#endif // DDSSECURITY

    if (!domainParticipant.get()) {
		TRAVODDS_LOG(LOG_ERROR, "Make DP Err");
		return nullptr;
	}
    domainPart = domainParticipant.get();

	if (qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != domainPart->enable()) {
			delete_participant(domainPart);
			domainPart = nullptr;
			TRAVODDS_LOG(LOG_ERROR, "Enable Err");
			return nullptr;
		}
	}

    participants_[domainParticipant->get_instance_handle()] = std::move(domainParticipant);

	return domainPart;
}

DomainParticipantFactoryImpl::~DomainParticipantFactoryImpl()
{
	participants_.clear();
#ifdef DDSSECURITY
    for (const auto& it : secPlugins_) {
        it.second.finalizeInstFunc();
    }
    secPlugins_.clear();
#endif // DDSSECURITY
}

ReturnCode_t DomainParticipantFactoryImpl::delete_participant(DomainParticipant*& participant)
{
    if (!participant) {
		TRAVODDS_LOG(LOG_ERROR, "RETCODE_BAD_PARAMETER");
        return RETCODE_BAD_PARAMETER;
    }
    InstanceHandle_t handle = participant->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxParticipants_);
    auto it = participants_.find(handle);
    if (it == participants_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "Handle is Err");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		TRAVODDS_LOG(LOG_ERROR, "DP is nullptr");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    participants_.erase(it);
    participant = nullptr;
    return RETCODE_OK;
}

DomainParticipant* DomainParticipantFactoryImpl::lookup_participant(DomainId_t domain_id) const
{
    DomainParticipant* dp = nullptr;
    for (auto& itor : participants_) {
        if (itor.second->get_domain_id() == domain_id) {
            dp = itor.second.get();
            break;
        }
    }
    return dp;
}

ReturnCode_t DomainParticipantFactoryImpl::set_default_participant_qos(const DomainParticipantQos& qos)
{
    if (&qos == &PARTICIPANT_QOS_DEFAULT) {
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
        return ret;
    }
    defaultParticipantQos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_default_participant_qos(DomainParticipantQos& qos) const
{
    qos = defaultParticipantQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::set_qos(const DomainParticipantFactoryQos& qos)
{

    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
        return ret;
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qos)) {
		TRAVODDS_LOG(LOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }
	qos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_qos(DomainParticipantFactoryQos& qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

DomainParticipantFactoryImpl* DomainParticipantFactoryImpl::get_instance()
{
	// 确保在获取实例时，获取程序的创建时间戳
	static std::string timestamp = SysInfo::GetExecutionTimestamp();
    /* 优先获取网络资源，决定网络资源最后释放 */
    static AsioContext& as = AsioContext::GetInstance();
    static std::shared_ptr<DomainParticipantFactoryImpl> Impl(new DomainParticipantFactoryImpl);
    return Impl.get();
}

void DomainParticipantFactoryImpl::finalize()
{
    get_instance()->set_qos(DomainParticipantFactoryQos());
    DomainParticipantQos qos;
    get_instance()->set_default_participant_qos(qos);
}

#ifdef DDSSECURITY

ReturnCode_t DomainParticipantFactoryImpl::load_security_plugin(
    const char* plugin_name,
    const char* file_name,
    const char* retrieve_instance_func_name,
    const char* finalize_instance_func_name)
{
    DomainParticipantFactoryImpl::SecurityPluginModule newModule;
    if (secPlugins_.find(plugin_name) != secPlugins_.end())
    {
        TRAVODDS_LOG(LOG_DEBUG, "%s existed.", plugin_name);
        return RETCODE_ERROR;
    }
#ifdef _WIN32
    HMODULE libModule = LoadLibraryA(file_name);
    if (libModule == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "%s load fail.", plugin_name);
        return RETCODE_ERROR;
    }
    // 找到函数接口地址
    GetSecurityPlugInstance getInstProc = (GetSecurityPlugInstance)GetProcAddress(libModule, retrieve_instance_func_name);
    if (getInstProc == NULL)
    {
        FreeLibrary(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Not find retrieve_instance_func_name in Plugin");
        return RETCODE_ERROR;
    }
    newModule.finalizeInstFunc = (FinalizeSecurityPlugInstance)GetProcAddress(libModule, finalize_instance_func_name);
    if (newModule.finalizeInstFunc == NULL)
    {
        FreeLibrary(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Not find finalize_instance_func_name in Plugin");
        return RETCODE_ERROR;
    }
    if (!getInstProc(&newModule.inst)) //创建实例
    {
        FreeLibrary(libModule);
        TRAVODDS_LOG(LOG_ERROR, "create PluginModule instance fail");
        return RETCODE_ERROR;
    }
    newModule.libModule = libModule;
#elif defined(_LINUX)
    void* libModule = dlopen(file_name, RTLD_LAZY);
    const char* errorInfo = NULL;
    if (libModule == NULL)
    {
        errorInfo = dlerror();
        TRAVODDS_LOG(LOG_ERROR, "Failed to load security library \"%s\": %s.", file_name, errorInfo);
        return RETCODE_ERROR;
    }
    GetSecurityPlugInstance getInstProc = (GetSecurityPlugInstance)dlsym(libModule, retrieve_instance_func_name);
    if (getInstProc == NULL || (errorInfo = dlerror()) != NULL)
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance retrieving function \"%s\": %s.",
            retrieve_instance_func_name, errorInfo);
        return RETCODE_ERROR;
    }
    newModule.finalizeInstFunc = (FinalizeSecurityPlugInstance)dlsym(libModule, finalize_instance_func_name);
    if (newModule.finalizeInstFunc == NULL || (errorInfo = dlerror()) != NULL)
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance finalize function \"%s\": %s.",
            finalize_instance_func_name, errorInfo);
        return RETCODE_ERROR;
    }
    if (!getInstProc(&newModule.inst))
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to retrieve security instance.");
        return RETCODE_ERROR;
    }
    newModule.libModule = libModule;
#elif defined(_REWORKS)
    GetSecurityPlugInstance getInstProc = NULL;
    SymType type;
    if (symbol_lkup(retrieve_instance_func_name, (void**)&getInstProc, &type) != 0)
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance retrieving function \"%s\".",
            retrieve_instance_func_name);
        return RETCODE_ERROR;
    }
    if (symbol_lkup(finalize_instance_func_name, (void**)&newModule.finalizeInstFunc, &type) != 0)
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance finalize function \"%s\".",
            finalize_instance_func_name);
        return RETCODE_ERROR;
    }
    if (!getInstProc(&newModule.inst))
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to retrieve security instance.");
        return RETCODE_ERROR;
    }
#elif defined(_VXWORKS_66)
    GetSecurityPlugInstance getInstProc = NULL;
    SYM_TYPE type;
    if (symFindByName(sysSymTbl, (char*)retrieve_instance_func_name, (char**)&getInstProc, &type) != OK)
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance retrieving function \"%s\".",
            retrieve_instance_func_name);
        return RETCODE_ERROR;
    }
    if (symFindByName(sysSymTbl, (char*)finalize_instance_func_name, (char**)&newModule.finalizeInstFunc, &type) != OK)
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance finalize function \"%s\".",
            finalize_instance_func_name);
        return RETCODE_ERROR;
    }
    if (!getInstProc(&newModule.inst))
    {
        TRAVODDS_LOG(LOG_ERROR, "Failed to retrieve security instance.");
        return RETCODE_ERROR;
    }
#else
    void* libModule = dlopen(file_name, RTLD_LAZY);
    const char* errorInfo = NULL;
    if (libModule == NULL)
    {
        errorInfo = dlerror();
        TRAVODDS_LOG(LOG_ERROR, "Failed to load security library \"%s\": %s.", file_name, errorInfo);
        return RETCODE_ERROR;
    }
    GetSecurityPlugInstance getInstProc = (GetSecurityPlugInstance)dlsym(libModule, retrieve_instance_func_name);
    if (getInstProc == NULL || (errorInfo = dlerror()) != NULL)
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance retrieving function \"%s\": %s.",
            retrieve_instance_func_name, errorInfo);
        return RETCODE_ERROR;
    }
    newModule.finalizeInstFunc = (FinalizeSecurityPlugInstance)dlsym(libModule, finalize_instance_func_name);
    if (newModule.finalizeInstFunc == NULL || (errorInfo = dlerror()) != NULL)
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to get security instance finalize function \"%s\": %s.",
            finalize_instance_func_name, errorInfo);
        return RETCODE_ERROR;
    }
    if (!getInstProc(&newModule.inst))
    {
        dlclose(libModule);
        TRAVODDS_LOG(LOG_ERROR, "Failed to retrieve security instance.");
        return RETCODE_ERROR;
    }
    newModule.libModule = libModule;
    TRAVODDS_LOG(LOG_DEBUG, "Ubuntu generate PluginModule");
#endif
    char* key = (char*)malloc(strlen(plugin_name) + 1);
    strcpy(key, plugin_name);
    (secPlugins_)[key] = newModule;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::GenerateGuidPrefix(int32_t instanceId, GUID_t& guid)
{
    int32_t processId = SysInfo::GetProcessIDStatic();

    //12 个字节
    GuidPrefix_t& prefix = guid.prefix;

    static std::random_device rd;
    static std::mt19937 generator(rd());  // 使用Mersenne Twister引擎
    static std::uniform_int_distribution<uint16_t> distribution(0, 65535);

    uint16_t rand_value = distribution(generator);
    uint16_t host_id = distribution(generator);

    prefix.value[0] = VENDORID_PROSIMA[0];
    prefix.value[1] = VENDORID_PROSIMA[1];

    char* temp = (char*)&host_id;
    prefix.value[2] = temp[0];
    prefix.value[3] = temp[1];

    temp = (char*)&processId;
    prefix.value[4] = temp[0];
    prefix.value[5] = temp[1];

    temp = (char*)&rand_value;
    prefix.value[6] = temp[0];
    prefix.value[7] = temp[1];

    temp = (char*)&instanceId;
    prefix.value[8] = temp[0];
    prefix.value[9] = temp[1];
    prefix.value[10] = temp[2];
    prefix.value[11] = temp[3];

    return RETCODE_OK;
}

#endif // DDSSECURITY
