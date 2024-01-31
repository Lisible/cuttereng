struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(1) texture_coordinates: vec2<f32>
}

@vertex
fn vs_main(
    @builtin(vertex_index) in_vertex_index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = vec4<f32>(f32(in_vertex_index & 1u) * 4.0 - 1.0, f32(in_vertex_index >> 1u) * 4.0 - 1.0, 0.0, 1.0);
    out.texture_coordinates = vec2<f32>(out.clip_position.xy * 0.5 + 0.5);
    return out;
}


struct GradientUniform {
    top_color: vec4<f32>,
    bottom_color: vec4<f32>
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
	var gradient_uniform: GradientUniform;
	gradient_uniform.top_color = vec4<f32>(0.6, 0.6, 1.0, 1.0);
	gradient_uniform.bottom_color= vec4<f32>(1.0, 0.4, 0.0, 1.0);
    return gradient_uniform.bottom_color * (1.0 - in.texture_coordinates.y * 2.0) + gradient_uniform.top_color * in.texture_coordinates.y * 2.0;
}

