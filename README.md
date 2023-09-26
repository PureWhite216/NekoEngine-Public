# NekoEngine
A Game Engine based on Vulkan.

![image](https://github.com/PureWhite216/NekoEngine-Public/blob/main/Images/Physics.png)
## Features
- 3D Rendering based on Vulkan.
- 3D RigidBody Physics Simulation
- ECS Gameplay Framework
- Lua script support

## Build

This project is built with [Xmake](https://xmake.io/#/about/introduction). 

It only supports **Windows** platform currently.

### 1. Install VulkanSDK

[Download the latest version](https://vulkan.lunarg.com/sdk/home#windows)

You should add **Volk Header** and **Vulkan Memory Allocation Header** when you install.

### 2. Install Xmake

[Installation - xmake](https://xmake.io/#/guide/installation)

On Windows，you can download the installer here: [Releases · xmake-io/xmake (github.com)](https://github.com/xmake-io/xmake/releases)

### 3. Run Xmake

You can download xmake plugin in your IDE(VScode, Clion...), and click "Build".

Or you can use the command:

```
xmake f -y -p windows -a x64 -m debug
```

### 4. Run the Application

```
xmake run
```

## Dependency

- Xmake
- VulkanSDK
- Entt
- GLFW
- OpenFBX
- ImGui
- Cereal
- Sol2
- MeshOptimizer
