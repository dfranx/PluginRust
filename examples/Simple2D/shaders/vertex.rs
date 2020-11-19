//! Ported to Rust from https://github.com/Tw1ddle/Sky-Shader/blob/master/src/shaders/glsl/sky.fragment

#![cfg_attr(target_arch = "spirv", no_std)]
#![feature(lang_items)]
#![feature(register_attr)]
#![register_attr(spirv)]

use spirv_std::glam::{Vec2, Vec4};
use spirv_std::{Input, Output};

#[allow(unused_attributes)]
#[spirv(vertex)]
pub fn main_vs(
	in_pos: Input<Vec2>,
    #[spirv(position)] mut out_pos : Output<Vec4>
) {
    out_pos.store(in_pos.load().extend(0.0).extend(1.0));
}

#[cfg(all(not(test), target_arch = "spirv"))]
#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! {
    loop {}
}

#[cfg(all(not(test), target_arch = "spirv"))]
#[lang = "eh_personality"]
extern "C" fn rust_eh_personality() {}