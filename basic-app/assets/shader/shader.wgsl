struct CommonUniforms {
    projection_matrix: mat4x4<f32>,
    current_time_secs: f32
}

struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) color: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

@group(0) @binding(0) var<uniform> u_common: CommonUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    var offset = vec2<f32>(-0.6875, -0.463);
    offset += 0.3  * vec2<f32>(cos(u_common.current_time_secs), sin(u_common.current_time_secs));
    out.position = vec4<f32>(in.position + offset, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}
