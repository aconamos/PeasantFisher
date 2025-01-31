use humantime;
use std::ffi::CStr;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn rel_time(text: *const c_char) -> u64 {
    let text = unsafe { CStr::from_ptr(text) };

    let text = text.to_str().expect("Conversion failed");

    match humantime::parse_duration(text) {
        Err(_) => 0,
        Ok(dur) => match dur.as_millis().try_into() {
            Err(_) => 0,
            Ok(ret) => ret,
        },
    }
}
