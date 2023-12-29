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


@group(0) @binding(0) var<uniform> u_common: CommonUniforms;
@group(1) @binding(0) var<uniform> u_mesh: MeshUniforms;

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
    var out: vec4<f32>;
    if (in_vertex_index == 0u) {
    out = vec4<f32>(-0.5, -0.5, 0.0, 1.0); 
    } else if (in_vertex_index == 1u) {
    out = vec4<f32>(0.5, -0.5, 0.0, 1.0); 
    } else {
    out = vec4<f32>(0.5, 0.5, 0.0, 1.0); 
    }
    return out;
}

struct TriangleOutput {
    @location(0) a: vec4<f32>,
}

@fragment
fn fs_main() -> TriangleOutput {
    var out: TriangleOutput;
    out.a = vec4<f32>(1.0, 1.0, 0.0, 1.0);
    return out;
}
