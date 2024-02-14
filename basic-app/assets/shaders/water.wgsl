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


struct Wave {
    direction: vec2<f32>,
    frequency: f32,
    amplitude: f32,
    phase: f32
}

fn sine(vertex: vec3<f32>, wave: Wave) -> f32 {
    let d = wave.direction;
    let xz = vertex.x * d.x + vertex.z * d.y;
    let t = u_common.current_time_secs * wave.phase;
    return wave.amplitude * sin(xz * wave.frequency + t);
}

fn compute_offset(vertex: vec3<f32>, wave: Wave) -> vec3<f32> {
    return vec3<f32>(0.0, sine(vertex, wave), 0.0);
}

fn compute_normal(vertex: vec3<f32>, wave: Wave) -> vec3<f32> {
    let d = wave.direction;
    let xz = vertex.x * d.x + vertex.z * d.y;
    let t = u_common.current_time_secs * wave.phase;
    let n = wave.frequency * wave.amplitude * d * cos(xz * wave.frequency + t);
    return vec3<f32>(n.x, n.y, 0.0);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let t = u_common.current_time_secs;

    var waves = array(
        Wave(normalize(vec2<f32>(-1.0, -1.0)), 14.0, 2.0/15.0, 6.0),
        Wave(normalize(vec2<f32>(0.0, -1.0)),  1.3, 2.0/13.0, 4.0),
        Wave(normalize(vec2<f32>(-1.0, 0.4)), 0.8, 2.0/10.0, 0.4),
        Wave(normalize(vec2<f32>(0.0, 0.8)), 0.4, 2.0/9.0, 0.3),
        Wave(normalize(vec2<f32>(0.0, -3.0)), 0.3, 2.0/7.0, 1.0),
        Wave(normalize(vec2<f32>(-1.0, 3.0)),  0.2, 2.0/5.0, 1.5),
        Wave(normalize(vec2<f32>(-1.0, 0.6)), 0.1, 2.0/2.0, 0.4),
        Wave(normalize(vec2<f32>(-1.0, 4.0)), 0.01, 2.0, 0.3)
    );
    let wave_count = 8;

    var world_pos = (u_mesh.world_from_local * vec4<f32>(in.position, 1.0));
    var h = vec3<f32>(0.0);
    var n = vec3<f32>(0.0);
    for (var i = 0; i < wave_count; i++) {
        h += compute_offset(world_pos.xyz, waves[i]);
        n += compute_normal(world_pos.xyz, waves[i]);
    }

    let new_pos = vec4<f32>(in.position, 1.0) + vec4<f32>(h, 0.0f);
    world_pos = u_mesh.world_from_local * new_pos;    
    out.world_position = vec4<f32>(0.0);
    out.position = u_common.projection_from_view * world_pos;
    out.texture_coordinates = in.texture_coordinates;
    out.normal = normalize(u_mesh.world_from_local * vec4<f32>(normalize(vec3<f32>(-n.x, 1.0, -n.y)), 1.0)).xyz;
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
    out.base_color = vec4<f32>(0.3, 0.4, 0.9, 1.0);
    out.normal = vec4<f32>(in.normal, 1.0);
    out.position = in.world_position;
    return out;
}
