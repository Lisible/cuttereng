struct DirectionalLight {
    direction: vec3<f32>,
}

struct CommonUniforms {
    projection_from_view: mat4x4<f32>,
    inverse_projection_from_view: mat4x4<f32>,
    directional_light: DirectionalLight,
    view_position: vec3<f32>,
    current_time_secs: f32,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var g_buffer_base_color_texture: texture_2d<f32>;
@group(1) @binding(1) var g_buffer_base_color_sampler: sampler;
@group(1) @binding(2) var g_buffer_normal_texture: texture_2d<f32>;
@group(1) @binding(3) var g_buffer_normal_sampler: sampler;
@group(1) @binding(4) var g_buffer_position_texture: texture_2d<f32>;
@group(1) @binding(5) var g_buffer_position_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) texture_coordinates: vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) in: u32) -> VertexOutput {
    var vertices = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, 1.0), 
        vec2<f32>(-1.0, -1.0), 
        vec2<f32>(1.0, -1.0), 
        vec2<f32>(1.0, -1.0), 
        vec2<f32>(1.0, 1.0), 
        vec2<f32>(-1.0, 1.0)
    );

    var texture_coordinates = array<vec2<f32>, 6>(
        vec2<f32>(0.0, 0.0), 
        vec2<f32>(0.0, 1.0), 
        vec2<f32>(1.0, 1.0), 
        vec2<f32>(1.0, 1.0), 
        vec2<f32>(1.0, 0.0), 
        vec2<f32>(0.0, 0.0)
    );
    var out: VertexOutput;
    out.position = vec4<f32>(vertices[in], 0.0, 1.0);
    out.texture_coordinates = texture_coordinates[in];
    return out;
}

fn compute_directional_light(light_direction: vec3<f32>, view_direction: vec3<f32>, normal: vec3<f32>, base_color: vec3<f32>) -> vec3<f32> {
    let light_dir = normalize(-light_direction);

    let diffuse_strength = max(dot(normal, light_dir), 0.0);
    let diffuse_color = base_color.rgb * diffuse_strength;

    let shininess = 8.0;
    let reflect_dir = reflect(-light_dir, normal);
    let specular_strength = pow(max(dot(view_direction, reflect_dir), 0.0), shininess);
    let specular_color = base_color.rgb * specular_strength;


    return diffuse_color + specular_color;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let base_color: vec4<f32>  = textureSample(g_buffer_base_color_texture, g_buffer_base_color_sampler, in.texture_coordinates);
    if base_color.a < 0.01 {
        discard;
    }
    var normal: vec3<f32>  = normalize(textureSample(g_buffer_normal_texture, g_buffer_normal_sampler, in.texture_coordinates).xyz);
    let frag_pos: vec3<f32>  = textureSample(g_buffer_position_texture, g_buffer_position_sampler, in.texture_coordinates).xyz;
    let view_pos =  u_common.view_position;
    let view_direction = normalize(view_pos - frag_pos);

    var result = base_color.rgb * 0.1;
    result += compute_directional_light(u_common.directional_light.direction, view_direction, normal, base_color.rgb);
    return vec4<f32>(result.rgb, 1.0);
}
