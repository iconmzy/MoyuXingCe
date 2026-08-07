# MoyuXingCe

MoyuXingCe 是一个 Windows 在线行测摸鱼练习器。应用通过 `balagk.com` 的公开页面和游客练习接口读取试卷与选择题，再用本地 Qt Widgets 界面展示；在线题库不会保存到本地数据库或 JSON 文件。

数据来源页面：<https://balagk.com/%E9%A2%98%E5%BA%93/>

## 当前功能

- 读取最新 50 套行测试卷
- 按网站公开的游客练习会话流程获取试卷题目
- 本地展示题干、材料和动态选项
- 提交答案后本地判题，或直接查看网站提供的标准答案与解析
- 按试卷保存已答选项、最后题号和正确率，重新加载后继续作答
- 默认显示工作台，可使用 `Ctrl+Space`，或按住 `Q` 再按 `E`，快速显示/隐藏数据审阅层
- 支持输入题号直接跳转，并可临时显示或隐藏试卷真实标题
- 设置菜单支持审阅层预览、透明度、字体大小、面板宽度和启动视图偏好
- 快速切换到本地备忘录
- 本地日志和窗口设置

网站会限制游客练习次数。应用会原样展示服务端的限制信息，不会绕过登录、会员或频率限制。网站接口或字段调整后，可能需要同步更新 `BalaApiClient`。

## 构建环境

- Windows 10/11 x64
- Visual Studio 2022
- CMake 3.25 或更高版本
- Qt 6.6 或更高版本，`MSVC 2022 64-bit`

在 Visual Studio Installer 中选择“使用 C++ 的桌面开发”，并确认安装：

- MSVC v143 C++ x64/x86 生成工具
- Windows 10 或 Windows 11 SDK
- 用于 Windows 的 C++ CMake 工具

Qt 只需要 Core、Gui、Widgets 和 Network。当前版本不需要 Qt SQL、Qt WebEngine、数据库驱动或 MinGW。

## 构建

将路径替换为实际 Qt 安装目录：

```powershell
cmake --preset windows-msvc-debug -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build --preset windows-msvc-debug
```

部署 Qt 运行时：

```powershell
windeployqt build/msvc-debug/Debug/KeepGongLearning.exe
```

## 本地数据

备忘录和应用日志写入 `%LOCALAPPDATA%` 下的应用目录；窗口设置、界面偏好和做题进度写入当前 Windows 用户设置。在线试卷和题目内容只存在于进程内存，关闭应用后即释放。



## 使用界面

![MoyuXingCe 使用界面](resources/imgs/%E5%BA%94%E7%94%A8%E7%95%8C%E9%9D%A2.JPG)





Tips：如果联网状态下无法加载数据集列表并打开的话，就是你们公司把这个在线网址ban了，只需要在启动程序的时候连接个人热点，打开一套卷子后就可以本地使用了。后续会考虑出纯离线的版本