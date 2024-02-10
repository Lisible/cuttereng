struct DirectionalLight {
    position: vec3<f32>,
}

struct CommonUniforms {
    projection_from_view: mat4x4<f32>,
    inverse_projection_from_view: mat4x4<f32>,
    light_space_projection_from_view_from_local: mat4x4<f32>,
    directional_light: DirectionalLight,
    view_position: vec3<f32>,
    current_time_secs: f32,
}

struct MeshUniforms {
    world_from_local: mat4x4<f32>,
}

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) texture_coordinates: vec2<f32>,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var<uniform> u_mesh: MeshUniforms;

@vertex
fn vs_main(in: VertexInput) -> @builtin(position) vec4<f32> {
	return u_common.light_space_projection_from_view_from_local * u_mesh.world_from_local * vec4<f32>(in.position, 1.0);
}
