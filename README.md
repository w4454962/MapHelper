# MapHelper
这是一个用于[YDWE](https://github.com/actboy168/YDWE)的加速保存插件.

​另外已经在这个项目的基础上 实现了 独立的ydwe的触发离线编译器程序
可以在不需要we的情况下 直接将 未编译的wtg wct数据 编译成 最终可运行的jass编译器
触发离线编译器尚未发布跟开源， 基本功能已经完毕 等将来适配好对应的gui界面后就会开放。

## 开发环境
* VS2022以上跟c++20支持 
* 修改输出文件路径
* 选择Release x86后正常编译

* 离线编译依赖 https://github.com/ddiakopoulos/libnyquist  

## 使用方式
* 编译出的dll修改为.asi放入魔兽目录，启动编辑器自动加载

支持覆盖替换掉ydtrigger.dll的

即在加载该插件的情况下 删除ydtrigger.dll 编辑器也能正常保存运行


## QQ群
* 群号 724829943 有问题的加群反馈 下载编译版本。  大概有6 700个地图作者使用过了，基本稳定没大问题。 


## 使用前后效果对比
原ydwe 保存半个小时的地图  使用后  只需要7 80秒

原ydwe 保存10分钟的地图    使用后  只需要30秒

原ydwe 保存1分钟的地图     使用后  只需要20秒 

体积越大，地形装饰物越多，触发器越多的地图 优化越明显。

