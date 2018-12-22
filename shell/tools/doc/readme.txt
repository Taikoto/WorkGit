项目编译：

正式量产版:
./eiot/tools/build_release.sh demo_test  mt8765

userdebu版:
./eiot/tools/build_userdebug.sh demo_test  mt8765

eng版:
./eiot/tools/eng.sh demo_test  mt8765



该代码删掉eiot目录后，就为mtk未做任何修改的源码！
我们修改的任何文件需要copy到eiot目录中

eiot目录结构：
eiot/driver  公共驱动
eiot/project 项目，其中结构为:eiot/project/平台/大项目名称/具体项目名
eiot/tools   编译相关的脚步
eiot/modems  存放所有项目的modem，编译的时候会把对应的modem拷贝到源码中
eiot/prebuild java预处理之前的文件，为了实现类似C语言宏预编译的处理，方便各种feature的通过开关控制
eiot/doc      说明文档
eiot/apps    自开发的有源码的app
eiot/preload_app 无源码的app
eiot/sepolicy  权限相关
eiot/rootdir  rc相关文件
