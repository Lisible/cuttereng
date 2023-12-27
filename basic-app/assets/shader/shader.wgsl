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
    @location(0) texture_coordinates: vec2<f32>,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var<uniform> u_mesh: MeshUniforms;
@group(2) @binding(0) var sand_texture: texture_2d<f32>;
@group(2) @binding(1) var sand_texture_sampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    var offset = vec2<f32>(-0.6875, -0.463);
    offset += 0.3  * vec2<f32>(cos(u_common.current_time_secs), sin(u_common.current_time_secs));
    out.position = u_common.projection_from_view * u_mesh.world_from_local * vec4<f32>(in.position, 1.0);
    out.texture_coordinates = in.texture_coordinates;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(textureSample(sand_texture, sand_texture_sampler, in.texture_coordinates).rgb,  1.0);
}
