@group(0) @binding(0) var directional_light_space_depth_map_texture: texture_2d<f32>;
@group(0) @binding(1) var directional_light_space_depth_map_sampler: sampler;

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


@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let depth_value: f32  = textureSample(directional_light_space_depth_map_texture, directional_light_space_depth_map_sampler, in.texture_coordinates).r;
    return vec4<f32>(vec3<f32>(depth_value), 1.0);
}
