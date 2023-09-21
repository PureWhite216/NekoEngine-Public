#pragma once

#include "cassert"
#include <cstdint>
#include "unordered_map"
#include "unordered_set"
#include "memory"
#include "string"
#include "vector"
#include "map"
#include "array"
#include "utility"
#include "iostream"
#include "functional"
#include "deque"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "queue"

#define NEKO_EXPORT __declspec(dllexport)

#define BIT(x) (1 << x)
#define LOG_FORMAT(format, ...) printf((format),  __VA_ARGS__); std::cout << std::endl
#define LOG(message) std::cout << message << std::endl
#define RUNTIME_ERROR(message) throw std::runtime_error(message)
#define ASSERT(condition, message) if(condition) throw std::runtime_error(message)
using String = std::string;
template <class T>
using SharedPtr = std::shared_ptr<T>;
template <class T>
using UniquePtr = std::unique_ptr<T>;
template <class T>
using WeakPtr = std::weak_ptr<T>;
#define MakeShared std::make_shared
#define MakeUnique std::make_unique
#define MakeWeak std::make_weak
template <class T1, class T2>
using Pair = std::pair<T1, T2>;
template <class T>
using ArrayList = std::vector<T>;
template <class T1, int T2>
using Array = std::array<T1, T2>;
template <class T1, class T2>
using HashMap = std::unordered_map<T1, T2>;
template <class T1, class T2>
using Map = std::map<T1, T2>;
template <class T>
using HashSet = std::unordered_set<T>;
template <class T>
using Deque = std::deque<T>;

using FVector2 = glm::vec2;
using FVector3 = glm::vec3;
using FVector4 = glm::vec4;
using Color = glm::vec4;
using Normal = glm::vec3;

