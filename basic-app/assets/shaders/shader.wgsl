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

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec4<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) texture_coordinates: vec2<f32>,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var<uniform> u_mesh: MeshUniforms;
@group(2) @binding(0) var u_material_base_color_texture: texture_2d<f32>;
@group(2) @binding(1) var u_material_base_color_sampler: sampler;
@group(2) @binding(2) var u_material_normal_texture: texture_2d<f32>;
@group(2) @binding(3) var u_material_normal_sampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.world_position =  (u_mesh.world_from_local * vec4<f32>(in.position, 1.0));
    out.position = u_common.projection_from_view * out.world_position;
    out.texture_coordinates = in.texture_coordinates;
    // out.normal = in.normal;
    out.normal =(u_mesh.world_from_local * vec4<f32>(in.normal, 0.0)).xyz;
    return out;
}

struct FragmentOutput {
    @location(0) base_color: vec4<f32>,
    @location(1) normal: vec4<f32>,
    @location(2) position: vec4<f32>,
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    out.base_color = textureSample(u_material_base_color_texture, u_material_base_color_sampler, in.texture_coordinates);
    out.normal = vec4<f32>(in.normal.xyz, 1.0);
    out.position = in.world_position;
    return out;
}
