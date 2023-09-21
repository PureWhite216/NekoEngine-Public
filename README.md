# NekoEngine
A Game Engine based on Vulkan.

## Features
- 3D Rendering based on Vulkan.
- 3D RigidBody Physics Simulation
- ECS Gameplay Framework
- Lua script support

## Build

This project is built with [Xmake](https://xmake.io/#/about/introduction). 

It only supports **Windows** platform currently.

### 1. Install Xmake

[Installation - xmake](https://xmake.io/#/guide/installation)

On Windows，you can download the installer here: [Releases · xmake-io/xmake (github.com)](https://github.com/xmake-io/xmake/releases)

### 2. Run Xmake

You can download xmake plugin in your IDE(VScode, Clion...), and click "Build".

Or you can use the command:

```
xmake f -y -p windows -a x64 -m debug
```

### 3. Run the application

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
