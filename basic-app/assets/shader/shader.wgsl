struct CommonUniforms {
    projection_from_view: mat4x4<f32>,
    current_time_secs: f32
}

struct MeshUniforms {
    world_from_local: mat4x4<f32>,
}

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) texture_coordinates: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var<uniform> u_mesh: MeshUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    var offset = vec2<f32>(-0.6875, -0.463);
    offset += 0.3  * vec2<f32>(cos(u_common.current_time_secs), sin(u_common.current_time_secs));
    out.position = u_common.projection_from_view * u_mesh.world_from_local * vec4<f32>(in.position, 1.0);
    // out.position = u_common.projection_from_view * u_mesh.world_from_local * vec4<f32>(in.position, 1.0);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 1.0, 0.0, 1.0);
}
