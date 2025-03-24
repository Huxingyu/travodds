#include "RTPS/Reader.h"

USING_AGIDDS_NAMESPACE


ReturnCode_t Reader::ReadOrTake(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int maxSamples,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	InstanceHandle_t handle = HANDLE_NIL;
	InstanceHandle_t nextHandle = HANDLE_NIL;
	ReturnCode_t ret = RETCODE_OK;
	
	/* 循环读取所有的Instance的数据 */
	do {
		ret = readerCache_.ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, nextHandle, sampleStates, viewStates, instanceStates, shouldTake);
		if (RETCODE_OK != ret) {
			break;
		}

		if (dataValues.length() > maxSamples) {
			break;
		}

		handle = nextHandle;
		ret = FindNextInstance(handle, nextHandle);	

	} while (RETCODE_ERROR != ret);

	if (0 >= dataValues.length()) {
		return RETCODE_NO_DATA;
	}
		
	return RETCODE_OK;
}

ReturnCode_t Reader::ReadOrTakeInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, int maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	ReturnCode_t ret = RETCODE_OK;
	/* 实例样本最新的状态  */
	SampleInfo info = sampleInfos_[handle];
	/* 实例样本在缓冲区最新数据的样本 */
	SampleInfo handleInfo;
	/* 实例样本读取到的最新状态 */
	SampleInfo readInfo;
	int size = 0;
	/* 读取 */
	if((0 != (viewStates & info.view_state))|| (0 != (instanceStates & info.instance_state)) ){
		return RETCODE_NO_DATA;
	}

	ret = readerCache_.ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, handle, sampleStates, viewStates, instanceStates, shouldTake);
	if (RETCODE_OK != ret) {
		return ret;
	}

	/* 修改状态  */
	size = sampleInfos.length();

	if (0 < size) {
		readInfo = sampleInfos[size - 1];
	}
	handleInfo = GetLastInstanceSampleInfo(handle);

	for (int i = 0; i < sampleInfos.length(); i++) {
		SampleInfo sampleInfo = sampleInfos[i];

		/*  从缓冲区读取时赋值状态  */
		//sampleInfo.sample_state;
		/*  在申明Cachce空间时，获取当前Instancehandle的SampleInfo的相关信息时赋值  */
		//sampleInfo.source_timestamp;
		//sampleInfo.instance_handle;
		//sampleInfo.publication_handle;
		//sampleInfo.disposed_generation_count;
		//sampleInfo.no_writers_generation_count;
		
		/* 更新为最新的状态 */
		sampleInfo.view_state = handleInfo.view_state;
		sampleInfo.instance_state = handleInfo.instance_state;

		/* 读取到的数据，读取到数据个数（剩余数据的个数） */
		sampleInfo.sample_rank = --size;

		/* (读取到最新sampleInfo.disposed_generation_count + 读取到最新SampleInfo.no_writers_generation_count) - 
			(当前sampleInfo.disposed_generation_count + 当前SampleInfo.no_writers_generation_count) */
		sampleInfo.generation_rank = (readInfo.disposed_generation_count + readInfo.no_writers_generation_count) -													(sampleInfo.disposed_generation_count + sampleInfo.no_writers_generation_count);

		/* (样本最新sampleInfo.disposed_generation_count + 样本最新SampleInfo.no_writers_generation_count) - 
			(当前sampleInfo.disposed_generation_count + 当前SampleInfo.no_writers_generation_count) */
		sampleInfo.absolute_generation_rank = (handleInfo.disposed_generation_count + handleInfo.no_writers_generation_count) -
												(sampleInfo.disposed_generation_count + sampleInfo.no_writers_generation_count);;

		/* 代表DataSample是否包含数据，或者仅用于传递实例instance_state的更改 */
		sampleInfo.valid_data = true;

	}

	/* 如果读取到的是实例的最后一个样本数据，将是实例的view状态修改为NOT_NEW_VIEW_STATE */
	//TODO  需要判断样本是最后一个数据 
	if (readInfo.reception_sequence_number == handleInfo.reception_sequence_number) {
		sampleInfos_[handle].view_state = NOT_NEW_VIEW_STATE;
	}

	return ret;
}

ReturnCode_t Reader::ReadOrTakeNextInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, int maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	InstanceHandle_t takeHandle = HANDLE_NIL;
	ReturnCode_t ret = RETCODE_ERROR;
	/* 从列表中获取大于handle的实例句柄，如果没有获取到说明没有其他数据 */
	ret = FindNextInstance(handle, takeHandle);
	if (RETCODE_OK != ret) {
		return ret;
	}
		
	return ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, takeHandle, sampleStates, viewStates, instanceStates, shouldTake);
}


ReturnCode_t Reader::FindNextInstance(InstanceHandle_t& preHander, InstanceHandle_t& instanceHandle)
{
	instanceHandle = HANDLE_NIL;

	//TODO
	/* 返回比preInstanceHander 大的InstanceHandle */


	/* 如果不存在比较大的 说明没有数据需要读取 */
	if (0) {
		return RETCODE_NO_DATA;
	}
	
	return RETCODE_OK;
}


CacheChange* Reader::new_change(ChangeKind_t kind, SerializedBuffer data, InstanceHandle_t handle,InstanceHandle_t pubHandle, Time_t sourceTimespace)
{
	CacheChange* cacheChange = nullptr;

	//cacheChange
	cacheChange->kind = kind;
	cacheChange->sampleInfo = sampleInfos_[handle];
	cacheChange->sequenceNumber = lastSequenceNumber_++;

	cacheChange->sampleInfo.publication_handle = pubHandle;
	cacheChange->sampleInfo.source_timestamp = sourceTimespace;

	return cacheChange;
}
SampleInfo Reader::GetLastInstanceSampleInfo(const InstanceHandle_t& handle)
{
	SampleInfo sampleInfo;
	sampleInfo.absolute_generation_rank = 0;

	return sampleInfo;
}

void Reader::ChangeSampleInfoByLiveliness()
{
	for (auto sampleInfo : sampleInfos_) {
		sampleInfo.second.no_writers_generation_count++;
	}
}


/* 远端的DW调用Dispose函数，修改对应instance实例的状态 */
void Reader::ChangeSampleInfoByDWDisposed(const InstanceHandle_t& handl)
{
	for (auto sampleInfo : sampleInfos_) {
		sampleInfo.second.disposed_generation_count++;
	}
	//TODO  begin修改History的中相关实例的的样本状态；
	
	// 将样本的状态修改为  NOT_ALIVE_DISPOSED_INSTANCE_STATE

	//TODO end
	


}
