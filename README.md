# travodds

#### 介绍
AGIROS开源社区自研DDS，研发开源数据分发通信中间件产品，实现完整的数据订阅和发布机制，提供15种QoS策略灵活配置；封装多种语言API接口，向上对接AGIROS提供功能接口；丰富配套工具链，界面化查询和访问统计信息；实现插件化架构，动态加载和卸载不同的功能模块；支持X86\ARM\Risc-V异构多平台，支持ubuntu22.04及以上、openEuler22.03及以上、国产麒麟主流操作系统。

#### 软件架构

    --cmake 编译
    --docs  文档
    --include
    	--common 	公共头文件
    	    --memory        内存、内存池
    	    --threads       线程
    	    --log	    日志	
    	    --time          时间
    	--dcps		dcps层头文件
    	    --domain         域模块
            --publish	     发布模块
   	    --subscribe	     订阅模块
   	    --topic          主题模块
		--base	          基础设施      
    		--listener       监听器模块
                --condition      条件等待集模块
    	--rtps 		rtps层头文件
    	    --endpoint       RTPS实体模块
            --discovery      发现模块
            --transport      传输模块
            --historycache   数据缓存模块
            --messages       报文封装解析模块
    	--qos       qos策略头文件
	--security  安全头文件
	--xml  	    xml配置解析头文件
	--cdr  	    序列化和反序列化头文件
	--type	    数据类型头文件

    --src
    	--common 	公共基础模块
    	    --memory        内存、内存池
    	    --threads       线程
    	    --log           日志模块
    	    --time	    时间
    	--dcps  	dcps模块
            --domain        域模块
            --publish	    发布模块
            --subscribe     订阅模块
            --topic         主题模块
    	    --base	    基础设施      
    	        --listener     监听器模块
                --condition    条件等待集模块
    	--rtps 		rtps模块
    	    --endpoint      RTPS实体模块
            --discovery     发现模块
            --transport     传输模块
            --historycache  数据缓存模块
            --messages      报文封装解析模块
            --qos         qos策略
    	--security    安全
    	--xml  	      xml配置解析
        --cdr  	      序列化和反序列化
    	--type	      数据类型	

    --test	测试框架与测试用例
    --example	示例
    --tools	脚本工具
    --thirdparty 第三方依赖
    --LICENSE
    --README.md
    --README.en.md



#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
