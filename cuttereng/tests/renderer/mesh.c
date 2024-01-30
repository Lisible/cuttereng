#include "../test.h"
#include <log.h>
#include <memory.h>
#include <renderer/mesh.h>

void t_mesh_init(void) {
  Vertex *vertices = allocator_allocate(&system_allocator, 3 * sizeof(Vertex));
  vertices[0] = (Vertex){
      .position = {0.0, 1.0, 0.0},
      .normal = {0.0, 0.0, -1.0},
      .texture_coordinates = {0.0, 0.4},
  };
  vertices[1] = (Vertex){.position = {1.0, 0.0, 0.0},
                         .normal = {0.0, 0.0, -1.0},
                         .texture_coordinates = {0.0, 0.4}};
  vertices[2] = (Vertex){.position = {0.0, 1.0, 0.0},
                         .normal = {0.0, 0.0, -1.0},
                         .texture_coordinates = {0.0, 0.4}};
  size_t vertex_count = 3;
  Index *indices = allocator_allocate(&system_allocator, 3 * sizeof(Index));
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  size_t index_count = 3;

  Mesh mesh;
  mesh_init(&mesh, vertices, vertex_count, indices, index_count);
  T_ASSERT_EQ(mesh_vertex_count(&mesh), 3);
  T_ASSERT_EQ(mesh_index_count(&mesh), 3);
  mesh_deinit(&system_allocator, &mesh);
}

TEST_SUITE(TEST(t_mesh_init))
