#pragma once

#include <cstdint>

namespace n64 {

// Minimal target-agnostic drawing surface. A concrete backend (OpenGL,
// Vulkan, a software rasterizer for tests) implements this. HLE graphics
// code below only ever talks to this interface, never to a specific API.
class GfxBackend {
 public:
  virtual ~GfxBackend() = default;

  virtual void SetViewport(int x, int y, int width, int height) = 0;
  virtual void BindTexture(uint32_t texture_id) = 0;
  virtual void DrawTriangle(const float* v0_xyz, const float* v1_xyz, const float* v2_xyz) = 0;
  virtual void Present() = 0;
};

}  // namespace n64
