@group(0) @binding(0) var g_buffer_base_color_texture: texture_2d<f32>;
@group(0) @binding(1) var g_buffer_base_color_sampler: sampler;
@group(0) @binding(2) var g_buffer_normal_texture: texture_2d<f32>;
@group(0) @binding(3) var g_buffer_normal_sampler: sampler;
@group(0) @binding(4) var g_buffer_position_texture: texture_2d<f32>;
@group(0) @binding(5) var g_buffer_position_sampler: sampler;

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
    let sample: vec4<f32>  = textureSample(g_buffer_base_color_texture, g_buffer_base_color_sampler, in.texture_coordinates);
    if sample.a < 0.01 {
        discard;
    }
    return vec4<f32>(sample.rgb, 1.0);
}
