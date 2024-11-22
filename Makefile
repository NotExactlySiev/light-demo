NAME	:= light

# Replace these with the prefix for your compile tools.
TGT 	:= mipsel-unknown-none-elf
MKISO	:= mkpsxiso
CC	:= $(TGT)-gcc
OBJCOPY	:= $(TGT)-objcopy

INCDIRS	:=
LIBDIRS	:=
LIBS	:=

# Do you want the spicyjpeg headers?
INCDIRS	+= -Ispicy

# Required flags
CCFLAGS	:= -Wall -mips1 -march=r3000 -mno-abicalls -static -fno-builtin -nostartfiles -msoft-float
LDFLAGS := -T ps-exe.ld -Wl,--oformat=elf32-littlemips

# User Flags
CCFLAGS	+= -g -O3 -flto -G0
LDFLAGS += -Wl,-Map=build/$(NAME).map

BUILD	:= build
LIBS	:= -lc

# Build a single executable or a disc image.
all: build/$(NAME).exe
#all: $(BUILD)/$(NAME).bin
#	mkdir -p $(BUILD)
.PHONY: all

SRCS	:= src/main.c src/light.c src/model.c src/gpu.c src/kernel.s src/math.c src/math.s
BINS	:= $(BUILD)/ball.elf $(BUILD)/cube.elf $(BUILD)/monke.elf $(BUILD)/weird.elf
build/$(NAME).elf: $(SRCS) $(BINS)
	$(CC) $(INCDIRS) $(CCFLAGS) $(LIBDIRS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/$(NAME).exe: $(BUILD)/$(NAME).elf
	$(OBJCOPY) -O binary $^ $@

$(BUILD)/%.elf: bin/%.ply
	$(TGT)-objcopy -I binary -O elf32-littlemips $^ $@


# keeping this around because I might wanna load .ply files from disc
#$(BUILD)/$(NAME).bin: disc.xml $(BUILD)/$(NAME).exe $(FILES)
#	$(MKISO) -y --quiet --cuefile build/$(NAME).cue --output $@ $<

# TODO: add a template for embedding binary data in minimal-psx

clean:
	rm -r build/*
.PHONY: clean
