WINDOWS_TARGET := x86_64-pc-windows-gnu
WINDOWS_RELEASE_EXE := target/$(WINDOWS_TARGET)/release/gtr2-mod-files.exe

.PHONY: build release windows windows-release run clean clean-windows clean-all test fmt check

build:
	cargo build

release:
	cargo build --release

windows:
	cargo build --target $(WINDOWS_TARGET)

windows-release:
	cargo build --release --target $(WINDOWS_TARGET)
	@printf 'Windows executable: %s\n' "$(WINDOWS_RELEASE_EXE)"

run:
	cargo run

clean:
	cargo clean

clean-windows:
	cargo clean --target $(WINDOWS_TARGET)

clean-all: clean clean-windows

test:
	cargo test

fmt:
	cargo fmt -- --check

check:
	cargo check

