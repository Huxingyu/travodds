# SM2-ID实现兼容性处理
## ubuntu openssl
1. 基础版本信息：
	```bash
	$ lsb_release -a
	No LSB modules are available.
	Distributor ID: Ubuntu
	Description:    Ubuntu 22.04.5 LTS
	Release:        22.04
	Codename:       jammy
	```
	```bash
	$ openssl version -a
	OpenSSL 3.0.2 15 Mar 2022 (Library: OpenSSL 3.0.2 15 Mar 2022)
	built on: Wed Feb  5 13:19:41 2025 UTC
	platform: debian-amd64
	options:  bn(64,64)
	compiler: gcc -fPIC -pthread -m64 -Wa,--noexecstack -Wall -Wa,--noexecstack -g -O2 -ffile-prefix-map=/build/openssl-rEtvJl/openssl-3.0.2=. -flto=auto -ffat-lto-objects -flto=auto -ffat-lto-objects -fstack-protector-strong -Wformat -Werror=format-security -DOPENSSL_TLS_SECURITY_LEVEL=2 -DOPENSSL_USE_NODELETE -DL_ENDIAN -DOPENSSL_PIC -DOPENSSL_BUILDING_OPENSSL -DNDEBUG -Wdate-time -D_FORTIFY_SOURCE=2
	OPENSSLDIR: "/usr/lib/ssl"
	ENGINESDIR: "/usr/lib/x86_64-linux-gnu/engines-3"
	MODULESDIR: "/usr/lib/x86_64-linux-gnu/ossl-modules"
	Seeding source: os-specific
	CPUINFO: OPENSSL_ia32cap=0xfefa32034f8bffff:0x400704219c27ab
	```
2. 无默认值
3. 命令行可通过设置参数"distid:1234567812345678" 使用默认ID，或使用其它指定的ID，举例如下：
	```bash
	openssl req -new -x509 ... -sigopt "distid:1234567812345678" ...
	openssl req -verify ... -vfyopt "distid:1234567812345678" # 只能验证certificate request, 
	```
4. 验证签名需要使用其它方法，使用 openssl verify -check_ss_sig 校验有ID的证书会返回失败(没有处理distid参数)
	```bash
	$ openssl verify -check_ss_sig -vfyopt "distid:1234567812345678" -CAfile root-ca.pem root-ca.pem
	C = CN, ST = street, L = locate, O = org, OU = depa, CN = name, emailAddress = test@mail
	error 7 at 0 depth lookup: certificate signature failure
	error root-ca.pem: verification failed
	805B519D87740000:error:06880006:asn1 encoding routines:ASN1_item_verify_ctx:EVP lib:../crypto/asn1/a_verify.c:217:
	```
5. 差异的代码(sm2_sign.c)：
	```CPP
	int ossl_sm2_compute_z_digest(uint8_t *out,
	                              const EVP_MD *digest,
	                              const uint8_t *id,
	                              const size_t id_len,
	                              const EC_KEY *key)
	{
		...
		// 直接使用id进行运算，如果没有id，则省略这步计算
		if (id_len > 0 && !EVP_DigestUpdate(hash, id, id_len)) {
		ERR_raise(ERR_LIB_SM2, ERR_R_EVP_LIB);
		goto done;
		}
		...
	}
	```

## openeuler openssl
1. 基础版本信息：
	```bash
	$ cat /etc/os-release
	NAME="openEuler"
	VERSION="24.03 (LTS)"
	ID="openEuler"
	VERSION_ID="24.03"
	PRETTY_NAME="openEuler 24.03 (LTS)"
	ANSI_COLOR="0;31"
	```
	```bash
	openssl version -a
	OpenSSL 3.0.12 24 Oct 2023 (Library: OpenSSL 3.0.12 24 Oct 2023)
	built on: Thu May 23 12:48:09 2024 UTC
	platform: linux-x86_64
	options:  bn(64,64)
	compiler: gcc -fPIC -pthread -m64 -Wa,--noexecstack -Wall -O3 -O2 -g -grecord-gcc-switches -pipe -fstack-protector-strong -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -Wp,-D_GLIBCXX_ASSERTIONS -specs=/usr/lib/rpm/generic-hardened-cc1 -m64 -mtune=generic -fasynchronous-unwind-tables -fstack-clash-protection -Wa,--noexecstack -Wa,--generate-missing-build-notes=yes -specs=/usr/lib/rpm/generic-hardened-ld -DOPENSSL_USE_NODELETE -DL_ENDIAN -DOPENSSL_PIC -DOPENSSL_BUILDING_OPENSSL -DZLIB -DNDEBUG -DPURIFY -DDEVRANDOM="\"/dev/urandom\""
	OPENSSLDIR: "/etc/pki/tls"
	ENGINESDIR: "/usr/lib64/engines-3"
	MODULESDIR: "/usr/lib64/ossl-modules"
	Seeding source: os-specific
	CPUINFO: OPENSSL_ia32cap=0xfefa32034f8bffff:0x400704219c27ab
	```

2. 默认ID为**"==1234567812345678=="**
	```cpp
	// sm2.h
	#define SM2_DEFAULT_USERID "1234567812345678"
	```
3. 命令行可通过设置参数"distid:some-id" 使用指定的ID
4. 使用 openssl verify -check_ss_sig 仅能校验默认ID的证书(没有处理distid参数，默认为"1234567812345678")
5. 差异的代码：
```cpp
int ossl_sm2_compute_z_digest(uint8_t *out,
                              const EVP_MD *digest,
                              const uint8_t *id,
                              const size_t id_len,
                              const EC_KEY *key)
{
    const uint8_t *f_id = id;
    size_t f_id_len = id_len;
	...
	/* if id is not set, use default id */
    if (f_id == NULL || f_id_len == 0) {
        f_id = (const uint8_t *)SM2_DEFAULT_USERID;
        f_id_len = strlen(SM2_DEFAULT_USERID);
    }
    ...
    if (f_id_len > 0 && !EVP_DigestUpdate(hash, f_id, f_id_len)) {
        ERR_raise(ERR_LIB_SM2, ERR_R_EVP_LIB);
        goto done;
    }	
	...
}
```

## GmSSL.org
1. 基础版本信息
	```
	http://gmssl.org/
	https://github.com/guanzhi/GmSSL/tree/master
	```
2. 默认ID为**"==1234567812345678=="**
3. 差异的代码(sm2_sign.c):
```cpp
int x509_certs_verify(const uint8_t *certs, size_t certslen, int certs_type,
	const uint8_t *rootcerts, size_t rootcertslen, int depth, int *verify_result)
{
	...
		if (x509_cert_verify_by_ca_cert(cert, certlen, cacert, cacertlen,
			SM2_DEFAULT_ID, SM2_DEFAULT_ID_LENGTH) != 1) {
			error_print();
			return -1;
		}
	...
}
```
```cpp
int sm2_verify_init(SM2_VERIFY_CTX *ctx, const SM2_KEY *key, const char *id, size_t idlen)
{
	if (id) {
		...
		sm2_compute_z(z, &key->public_key, id, idlen);
		sm3_update(&ctx->sm3_ctx, z, sizeof(z));
	}
}
```

## GmSSL.cn
1. 未开源，仅能从定义和调用方式了解有默认ID (ec.h)
	```cpp
	/* The default user id as specified in GM/T 0009-2012 */
	#  define SM2_DEFAULT_USERID "1234567812345678"
	```
2.  使用网站中签发的证书，经过校验，使用默认ID对对证书链进行了签名。
	```
	签发网站： https://www.gmcrt.cn/gmcrt/index.jsp
	```
	```bash
	$ openssl verify -check_ss_sig -CAfile sm2.ca.pem sm2.ca.pem
	sm2.ca.pem: OK
	$ openssl verify -check_ss_sig -CAfile sm2.ca.pem sm2.test.sig.crt.pem
	sm2.test.sig.crt.pem: OK
	```

## Tongsuo 铜锁
1. 基础版本信息
	```
	https://tongsuo.net/
	https://github.com/Tongsuo-Project/Tongsuo/tree/master
	```
2. 默认ID为**"==1234567812345678=="**
3. 差异的代码(sm2_sign.c):
```cpp
int ossl_sm2_compute_z_digest(uint8_t *out,
                              const EVP_MD *digest,
                              const uint8_t *id,
                              size_t id_len,
                              const EC_KEY *key)
{
	...
    if (id == NULL) {
        id = (const uint8_t *)SM2_DEFAULT_USERID;
        id_len = strlen(SM2_DEFAULT_USERID);
    }
	...
}
```

## 汇总

| 实现版本                  | 操作系统/平台             | OpenSSL版本 | 默认ID | 命令行ID参数           | 签名验证支持    | 代码实现差异点                             | 兼容处理          |
| --------------------- | ------------------- | --------- | ---- | ----------------- | --------- | ----------------------------------- | ------------- |
| **Ubuntu OpenSSL**    | Ubuntu 22.04.5 LTS  | 3.0.2     | ❌    | ✅ 支持 `distid:xxx` | ❌ 验证失败    | 直接使用传入ID，无ID时省略计算                   | 需要补充默认ID的兼容处理 |
| **openEuler OpenSSL** | openEuler 24.03 LTS | 3.0.12    | ✅    | ✅ 支持 `distid:xxx` | ✅ 仅支持默认ID | ID为空或长度为零时使用默认ID                    | 兼容            |
| **GmSSL.org**         | 跨平台                 | 独立实现      | ✅    | ❌ 不支持自定义ID        | ✅ 完全支持    | 命令行/TLS使用默认ID，lib接口支持ID参数，ID为空时省略计算 | 兼容            |
| **GmSSL.cn**          | 商业版本                | 未开源       | ✅    | ✅ 支持自定义ID         | ✅ 完全支持    | 符合GM/T 0009-2012标准???               | 未知            |
| **Tongsuo 铜锁**        | 跨平台                 | 独立实现      | ✅    | ✅ 支持自定义ID         | ✅ 完全支持    | ID为空时使用默认ID                         | 兼容            |



## 商业证书使用默认ID情况汇总



### 根证书

| # | 序列号 | CA Issuer | Subject | Certificate File | 使用默认ID |
|---|---------------|-----------|---------|------------------|------------|
| 1 | 17F5E3C18B7E1... | NRCAC-Device ROOT | NRCAC-Device ROOT | 设备应用根证书.pem | ✅ |
| 2 | 2BB5B8C349B03... | NRCAC-ROOTCA | NRCAC-ROOTCA | 社会公众应用根证书（SM2）.pem | ✅ |
| 3 | 62A370EE2A688... | NRCAC-Civil Servant ROOT | NRCAC-Civil Servant ROOT | 公务人员应用根证书.pem | ✅ |

  

### CA证书


| # | 序列号 | CA Issuer | Subject | Certificate File | 使用默认ID |
|---|---------------|-----------|---------|------------------|------------|
| 1 | 13760B1AD006A... | NRCAC-ROOTCA | GOTEC-TKCA SM2 | 北京中交国通智能交通系统技术有限公司_sm2.pem | ✅ |
| 2 | 1521B0BF43A84... | NRCAC-ROOTCA | Inner Mongolia Network Trus... | 内蒙古网信电子认证有限责任公司_sm2.pem | ✅ |
| 3 | 15C0DA492108D... | NRCAC-ROOTCA | 湖南省数字认证服务中心有限公司-HUNANCASM2 | 湖南省数字认证服务中心有限公司_sm2.pem | ✅ |
| 4 | 1E607BB74148C... | NRCAC-ROOTCA | WoTrus CA-WoTrus SM2 CA | 沃通电子认证服务有限公司_sm2.pem | ✅ |
| 5 | 1F35D919056CE... | NRCAC-ROOTCA | Zhejiang Digital Certificat... | 浙江省数字安全证书管理有限公司_sm2.pem | ✅ |
| 6 | 1F5D040688150... | NRCAC-ROOTCA | China Financial Certificati... | 中金金融认证中心有限公司_sm2.pem | ✅ |
| 7 | 1FAEBF893E64C... | NRCAC-ROOTCA | SuperTech ECardCA-SuperTech... | 苏博云科数字认证有限公司_sm2.pem | ✅ |
| 8 | 24B66235B6E98... | NRCAC-ROOTCA | GXCA-Guangxi SM2 CA | 广西壮族自治区数字证书认证中心有限公司_sm2.pem | ✅ |
| 9 | 25F8C2491EBAA... | NRCAC-ROOTCA | ShanXi Digital Certificate ... | 山西省数字证书认证中心（有限公司）_sm2.pem | ✅ |
| 10 | 2665D34E9B226... | NRCAC-ROOTCA | 黑龙江省数字证书认证有限公司-HLJCA_SM2 | 黑龙江省数字证书认证有限公司_sm2.pem | ✅ |
| 11 | 26EBE3B52E96A... | NRCAC-ROOTCA | 新疆数字证书认证中心-XJCASM2 | 新疆数字证书认证中心（有限公司）_sm2.pem | ✅ |
| 12 | 272202B826371... | NRCAC-Civil Servant ROOT | 中国电子口岸-中国电子业务证书管理中心 | 中国电子口岸数据中心_sm2.pem | ✅ |
| 13 | 27337ACC2239A... | NRCAC-Civil Servant ROOT | 人力资源和社会保障部-人力资源和社会保障部 | 人力资源和社会保障部信息中心_sm2.pem | ✅ |
| 14 | 2734BED2E147C... | NRCAC-Civil Servant ROOT | Fujian Digital Certificate ... | 福建省数字安全证书管理有限公司_sm2.pem | ✅ |
| 15 | 28265F9116CD7... | NRCAC-ROOTCA | CICV-CICV SM2 | 国汽=北京=智能网联汽车研究院有限公司_sm2.pem | ✅ |
| 16 | 2B01856C672FB... | NRCAC-ROOTCA | ShaanxiDigitalCertificateAu... | 陕西省数字证书认证中心有限责任公司_sm2.pem | ✅ |
| 17 | 2D4409FB64D21... | NRCAC-ROOTCA | EPRI-SGCCCA | 中国电力科学研究院有限公司_sm2.pem | ✅ |
| 18 | 2D7AD9F502BE6... | NRCAC-ROOTCA | CAICT-Taier CA | 泰尔认证中心有限公司_sm2.pem | ✅ |
| 19 | 2DA1A6BD0CE76... | NRCAC-ROOTCA | UniTrust-SHECA SM2 | 上海市数字证书认证中心有限公司_sm2.pem | ✅ |
| 20 | 30F8F864E55DF... | NRCAC-ROOTCA | Anicert Corporation-CTIDCA ... | 北京中盾安信科技发展有限公司_sm2.pem | ✅ |
| 21 | 349CE5AB405E2... | NRCAC-ROOTCA | Sinorail Certification Auth... | 中铁信弘远=北京=软件科技有限责任公司_sm2.pem | ✅ |
| 22 | 34B1EEE2DA245... | NRCAC-ROOTCA | 江苏公众数科数字科技有限公司-GZCA_SM2 | 江苏公众数科数字科技有限公司_sm2.pem | ✅ |
| 23 | 37DAA552864EF... | NRCAC-ROOTCA | CMCA Root CA | 卓望数码技术=深圳=有限公司_sm2.pem | ✅ |
| 24 | 381EE63A88548... | NRCAC-ROOTCA | 北京天威诚信电子商务服务有限公司-天威诚信数字认证中心根CA | 北京天威诚信电子商务服务有限公司_sm2.pem | ✅ |
| 25 | 389E8FF17B178... | NRCAC-ROOTCA | Shandong Zhixin Certificati... | 山东豸信认证服务有限公司_sm2.pem | ✅ |
| 26 | 393556015E364... | NRCAC-ROOTCA | Hainan XinAn Electronic Cer... | 海南省信安电子认证有限公司_sm2.pem | ✅ |
| 27 | 39BF7C1F8170F... | NRCAC-ROOTCA | ShenZhen Certificate Author... | 深圳市电子商务安全证书管理有限公司_sm2.pem | ✅ |
| 28 | 3B38865BB54DE... | NRCAC-ROOTCA | NELDTV-GSCA | 深圳数字电视国家工程实验室股份有限公司_2_sm2.pem | ✅ |
| 29 | 3C4AF5224F0F3... | NRCAC-ROOTCA | Sichuan Digital Certificate... | 四川省数字证书认证管理中心有限公司_sm2.pem | ✅ |
| 30 | 3E7E54DFDC3F7... | NRCAC-ROOTCA | GUANG DONG CERTIFICATE AUTH... | 数安时代科技股份有限公司_sm2.pem | ✅ |
| 31 | 42028B4040F87... | NRCAC-Civil Servant ROOT | 税务电子证书一级根(SM2) | 国家税务总局电子税务管理中心_sm2.pem | ✅ |
| 32 | 429DFF70034C7... | NRCAC-ROOTCA | RCBFCC SM2-RCBFCC SM2 CACHILD | 农信银资金清算中心有限责任公司_sm2.pem | ✅ |
| 33 | 42F719E748709... | NRCAC-Civil Servant ROOT | 天津市大数据管理中心-TJCA | 天津市大数据管理中心_sm2.pem | ✅ |
| 34 | 446A952879599... | NRCAC-Civil Servant ROOT | 公安部第三研究所-GACA | 公安部第三研究所_sm2.pem | ✅ |
| 35 | 451D0E10D2E33... | NRCAC-ROOTCA | SINOCIPHER-AIOTCA | 中安云科认证技术（山东）有限公司_sm2.pem | ✅ |
| 36 | 45282FC0236C3... | NRCAC-Civil Servant ROOT | National E-Government Netwo... | 国家信息中心_sm2.pem | ✅ |
| 37 | 4694C1AE51E16... | NRCAC-ROOTCA | BambooCloud Service Co.-Bam... | 深圳竹云科技股份有限公司_sm2.pem | ✅ |
| 38 | 4899D4594F6AB... | NRCAC-ROOTCA | East-Zhongxun Certificate A... | 东方中讯数字证书认证有限公司_sm2.pem | ✅ |
| 39 | 48F51BBCF9036... | NRCAC-ROOTCA | ZHCA-ZHCA | 天津市中环认证服务有限公司_sm2.pem | ✅ |
| 40 | 4974989589A7E... | NRCAC-ROOTCA | Esign Digital Certificate A... | 杭州天谷信息科技有限公司_sm2.pem | ✅ |
| 41 | 49AD97D1D672F... | NRCAC-ROOTCA | TrustAsia Technologies-Trus... | 亚数信息科技=上海=有限公司_sm2.pem | ✅ |
| 42 | 4AD08A37BE61D... | NRCAC-ROOTCA | AnHui Certification Authori... | 安徽省电子认证管理中心有限责任公司_sm2.pem | ✅ |
| 43 | 4C7F0DDFDB225... | NRCAC-ROOTCA | HeNan Certificate Authority... | 华测电子认证有限责任公司_sm2.pem | ✅ |
| 44 | 4E581178CDAEB... | NRCAC-Device ROOT | NELDTV-GSCA SM2 | 深圳数字电视国家工程实验室股份有限公司_sm2.pem | ✅ |
| 45 | 4E84F6B4A47B9... | NRCAC-ROOTCA | CWCA-CWCA | 西部安全认证中心有限责任公司_sm2.pem | ✅ |
| 46 | 53E68AC6AA8A2... | NRCAC-ROOTCA | 江苏国密数字认证有限公司-GMCA SM2 GM Au... | 江苏国密数字认证有限公司_sm2.pem | ✅ |
| 47 | 567EE5D24000E... | NRCAC-ROOTCA | MCSCA-MCSCA | 大陆云盾电子认证服务有限公司_sm2.pem | ✅ |
| 48 | 57B4EE07661C2... | NRCAC-ROOTCA | ynca-YNCA_SM2 | 云南省数字证书认证中心有限公司_sm2.pem | ✅ |
| 49 | 57CC0EBB04819... | NRCAC-ROOTCA | CIECC-ROOT CERTIFICATE FOR ... | 北京国富安电子商务安全认证有限公司_sm2.pem | ✅ |
| 50 | 57EC5F6D4B6E3... | NRCAC-ROOTCA | HeNan Province Information ... | 河南省信息化发展有限公司_sm2.pem | ✅ |
| 51 | 588ED2A4FAA03... | NRCAC-ROOTCA | Fujian Digital Certificate ... | 福建省数字安全证书管理有限公司_sm2_1.pem | ✅ |
| 52 | 59754DAA8F9D8... | NRCAC-ROOTCA | DongFang Electronic Certifi... | 东方新诚信数字认证中心有限公司_sm2.pem | ✅ |
| 53 | 5BA1A3726ED21... | NRCAC-ROOTCA | GuiZhou Electronic Certific... | 贵州省电子证书有限公司_sm2_1.pem | ✅ |
| 54 | 5CB0E9033DFDB... | NRCAC-ROOTCA | 群盾科技-QDCA SM2 CA | 北京群盾科技有限公司_sm2.pem | ✅ |
| 55 | 5D1AA8E2465F5... | NRCAC-ROOTCA | JSGXCA-JSGXCA_SM2 | 江苏省国信数字科技有限公司_sm2.pem | ✅ |
| 56 | 620A7B7968E3E... | NRCAC-ROOTCA | CQC-CQC CA | 北京中认环宇信息安全技术有限公司_sm2.pem | ✅ |
| 57 | 62BB1C8224F9D... | NRCAC-ROOTCA | Hubei Digital Certificate A... | 湖北省数字证书认证管理中心有限公司_sm2.pem | ✅ |
| 58 | 64CB1265D44A6... | NRCAC-ROOTCA | Ant Financial-Ant Financial... | 蚂蚁科技集团股份有限公司_sm2.pem | ✅ |
| 59 | 6557D8E5FE70F... | NRCAC-ROOTCA | JXCA-JXCA | 江西省数字证书有限公司_sm2.pem | ✅ |
| 60 | 65EF10AF4C16D... | NRCAC-ROOTCA | LNCA-LNCA SM2 | 辽宁数字证书认证管理有限公司_sm2.pem | ✅ |
| 61 | 66A1AF8B4BAA9... | NRCAC-ROOTCA | NETCA Certificate Authority... | 广东省电子商务认证有限公司_sm2.pem | ✅ |
| 62 | 681FC586C9980... | NRCAC-Civil Servant ROOT | 12100000K00002351Y-GB CA | 国务院办公厅机关服务中心_sm2.pem | ✅ |
| 63 | 6955523AA532B... | NRCAC-ROOTCA | TJCA-TJCA | 天津市滨海数字认证有限公司_sm2.pem | ✅ |
| 64 | 6B1C6FA0CD87D... | NRCAC-ROOTCA | NJCA-NJCA | 南京数字认证有限公司_sm2.pem | ✅ |
| 65 | 6B3E90C1520A5... | NRCAC-Civil Servant ROOT | 江苏省电子政务数字证书-江苏省电子政务证书认证中心 | 江苏省电子政务证书认证中心_sm2.pem | ✅ |
| 66 | 6CBE2196BD50D... | NRCAC-ROOTCA | 北京世纪速码信息科技有限公司-CSCA | 北京世纪速码信息科技有限公司_sm2.pem | ✅ |
| 67 | 6E7FB37745B87... | NRCAC-ROOTCA | CHINAUNICOM-CHINAUNICOM SM2 CA | 联通智慧安全科技有限公司_sm2.pem | ✅ |
| 68 | 70D4B1AB743A6... | NRCAC-Device ROOT | 交通运输部-MOTCA_Device | 交通运输部公路科学研究所_sm2_1.pem | ✅ |
| 69 | 71C67C042B5BF... | NRCAC-ROOTCA | 江苏智慧数字认证有限公司-江苏智慧数字认证有限公司 | 江苏智慧数字认证有限公司_sm2.pem | ✅ |
| 70 | 72B09F746784F... | NRCAC-ROOTCA | ECCA.COM.CN-SM2RootCA | 颐信科技有限公司_sm2.pem | ✅ |
| 71 | 7336EB0419FC0... | NRCAC-ROOTCA | CATARC-CATARC CA | 中汽数据=天津=有限公司_sm2.pem | ✅ |
| 72 | 75DFC8554A8C0... | NRCAC-ROOTCA | BJCA-Beijing SM2 CA | 北京数字认证股份有限公司_sm2.pem | ✅ |
| 73 | 76756E2911812... | NRCAC-ROOTCA | AnXin CA-AnXin SM2 CA | 吉林省安信电子认证服务有限公司_sm2.pem | ✅ |
| 74 | 767757CA00F0F... | NRCAC-Civil Servant ROOT | 教育部-National Education CA | 教育部教育管理信息中心_sm2.pem | ✅ |
| 75 | 787352C6CE4EB... | NRCAC-ROOTCA | 贵州省电子证书有限公司-GZCAOPERROOT | 贵州省电子认证科技有限公司_sm2_2.pem | ✅ |
| 76 | 78AAFFDC5A9D6... | NRCAC-ROOTCA | iFuture Service Co.-iFuture... | 重庆程远未来电子商务服务有限公司_sm2.pem | ✅ |
| 77 | 7949C31D22632... | NRCAC-Civil Servant ROOT | 交通运输部-MOTCA_Civil Servant | 交通运输部公路科学研究所_sm2.pem | ✅ |
| 78 | 79E95AD95B7A8... | NRCAC-ROOTCA | SDCA-SDSM2CA | 山东省数字证书认证管理有限公司_sm2_1.pem | ✅ |
| 79 | 7A5026480CC25... | NRCAC-ROOTCA | SDCA-ShanDongSM2CA | 山东省数字证书认证管理有限公司_sm2.pem | ✅ |



## 关键发现

### 1. 默认ID标准
- **标准值**: `1234567812345678` (符合GM/T 0009-2012国密标准???)
- **支持情况**: 除Ubuntu OpenSSL外，所有实现都支持此默认ID

### 2. 兼容性问题
- **Ubuntu OpenSSL**: 最大的兼容性问题，无默认ID且验证失败
- **openEuler OpenSSL**: 部分兼容，仅支持默认ID验证
- **其他实现**: 完全兼容，支持自定义和默认ID

### 3. 技术实现差异
- **Ubuntu**: 最简实现，无默认值处理
- **openEuler**: 添加默认ID处理逻辑
- **GmSSL/Tongsuo**: 完整的SM2实现，包含优化

### 4. 商业证书

- 根证书及CA证书均使用了默认ID。

- 暂未发现其它ID的证书。

  
