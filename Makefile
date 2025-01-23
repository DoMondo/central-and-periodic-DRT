#OPENGL_TARGET?=opencl
OPENGL_TARGET?=opengl
SCHED?=auto_sched
N?=1024
ARCH?=host
GENERATORS = $(basename $(wildcard common/generators/*.cpp))
GENERATED_LIBS = $(addsuffix .a, $(addprefix build/$N/$(ARCH)/generated/$(SCHED)/, \
	$(notdir $(GENERATORS))))
GENERATOR_BINS =$(addprefix build/$N/generators/, $(notdir $(GENERATORS)))
COMMON_FILES := $(wildcard common/*.cpp)
LINUX_FILES = $(wildcard platforms/linux/*.cpp)
ARM_FILES = $(wildcard platforms/arm-standalone/*.cpp)
DEPS:=$(GENERATOR_BINS:=.d)
ARM_SYSROOT := /opt/arm64-standalone-toolchain/sysroot
HOST_BUILD = false
ifneq (,$(findstring host,$(ARCH)))
	HOST_BUILD = true
endif
ifeq ($(HOST_BUILD), true)
	EXE_FILES := $(LINUX_FILES)
	CPP := g++
	CPP_FLAGS := \
		-I$(HALIDE_ROOT)/include \
		-std=c++11 \
	  	-ldl `libpng-config --cflags --ldflags` \
		-lGLU -lGL \
		-ljpeg -lpthread -lX11 \
		-L$(HALIDE_ROOT)/lib \
		-lHalide
else 
	EXE_FILES := $(ARM_FILES)
	CPP := /opt/arm64-standalone-toolchain/bin/aarch64-linux-android-clang++
	CPP_FLAGS:= \
		-L build/$N/$(ARCH)/generated/$(SCHED) \
		-L$(HALIDE_ROOT)/include \
		-L$(HALIDE_ROOT)/bin \
		-I $(HALIDE_ROOT)/tools \
		-I $(HALIDE_ROOT)/include \
		-I build/$N/$(ARCH)/generated/$(SCHED) \
		--sysroot=$(ARM_SYSROOT) \
		-std=c++11  \
		-lm -llog -landroid -latomic -lEGL -fPIE -pie \
	   -static-libstdc++ -D__ANDROID__ -ldl 
endif
EXE_NAME = radon-halide-$N-$(ARCH)-$(SCHED)

GENGEN_PATH = $(HALIDE_ROOT)/share/Halide/tools/GenGen.cpp

SCHED_PARAMS?=machine_params=16,16777216,0
SCHED_PARAMS?=machine_params=4,6291456,0

$(EXE_NAME): $(EXE_FILES) $(GENERATED_LIBS)
	@echo Generating $(EXE_NAME)
	@$(eval MACROS="-DVAL_N=$N")
	@$(CPP) $^ -g -I $(HALIDE_ROOT)/share/Halide/tools \
		  $(CPP_FLAGS) $(MACROS) \
		  -I build/$N/$(ARCH)/generated/$(SCHED)/ \
		  -o $(EXE_NAME)

# Generate .a
$(GENERATED_LIBS) : build/$N/$(ARCH)/generated/$(SCHED)/%.a: build/$N/generators/%
	@mkdir -p build/$N/$(ARCH)/generated
	@mkdir -p build/$N/$(ARCH)/generated/$(SCHED)
ifeq ($(SCHED),auto_sched)
	@echo Generating auto sched $@ 
	@#echo Sched params used are $(SCHED_PARAMS)
	@LD_LIBRARY_PATH=$(HALIDE_ROOT)/lib \
		$< \
		-o $(dir $@) \
		-g $(notdir $(basename $@)) \
		-p $(HALIDE_ROOT)/lib/libautoschedule_adams2019.so \
		-s Adams2019 \
		target=$(ARCH) auto_schedule=true $(SCHED_PARAMS)
else ifeq ($(SCHED),manual_sched)
	@echo Generating nosched $@
	@LD_LIBRARY_PATH=$(HALIDE_ROOT)/lib \
		$< \
	  	-o $(dir $@) \
		-g $(notdir $(basename $@)) \
		target=$(ARCH) auto_schedule=false
else
	$(ERROR: SCHED=$(SCHED) Please define the variable SCHED correctly (auto_sched or manual_sched))
endif

# Generate GenGen obj
build/GenGen.o:
	@mkdir -p build/
	@echo building GenGen obj N = $N
	@g++ -c $(GENGEN_PATH) -std=c++11 -fno-rtti -I$(HALIDE_ROOT)/include -g -o $@

-include $(DEPS)

# Generate Generators
$(GENERATOR_BINS): build/$N/generators/%: common/generators/%.cpp build/GenGen.o
	@mkdir -p build/$N/generators/
	@$(eval var=$(shell echo 'l($N)/l(2) + 1' | bc -l | sed -e 's|\..*||g'))
	@$(eval MAX_IT=$(shell echo $(var)))
	@$(eval MACROS=-DVAL_N=$N -DMAX_IT=$(MAX_IT))
	@echo Generating obj $@
	@# Generate object (MMD is necesary for the dependencies (REDUNDANT))
	@g++ -c $< $(MACROS) -I$(HALIDE_ROOT)/include \
	  	-std=c++11 -fno-rtti -lpthread -MMD -o $@
	@rm $@
	@echo Linking $< with GenGen  
	@# Link with the GenGen
	@g++ $(MACROS) \
		 $(basename $<).cpp build/GenGen.o \
		 -I$(HALIDE_ROOT)/include\
		-std=c++11 -fno-rtti -g  \
		-L$(HALIDE_ROOT)/lib \
		-lHalide -lz -ldl -lpthread \
		-o $@



clean:
	rm -rf build radon-halide* platforms/android/gradle_build


arm64-run-sched-profile: 
	$(eval ARCH = arm-64-profile)
	$(eval SCHED = auto_sched)
	$(eval SCHED_PARAMS = machine_params=4,1048576,10)
	@make ARCH=$(ARCH) SCHED=$(SCHED) SCHED_PARAMS=$(SCHED_PARAMS) 
	adb push $(EXE_NAME) /data/local/tmp
	adb shell /data/local/tmp/$(EXE_NAME) 

arm64-run-sched: 
	$(eval ARCH = arm-64-android)
	$(eval SCHED = auto_sched)
	$(eval SCHED_PARAMS = machine_params=4,1048576,10)
	@make ARCH=$(ARCH) SCHED=$(SCHED) SCHED_PARAMS=$(SCHED_PARAMS) 
	adb push $(EXE_NAME) /data/local/tmp
	adb shell /data/local/tmp/$(EXE_NAME) 

arm64-run-nosched: 
	$(eval ARCH = arm-64-android)
	$(eval SCHED = manual_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 
	adb push $(EXE_NAME) /data/local/tmp
	adb shell /data/local/tmp/$(EXE_NAME) 

arm64-build-gpu: 
	$(eval ARCH = arm-64-android-$(OPENGL_TARGET))
	$(eval SCHED = manual_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 

arm64-run-gpu: arm64-build-gpu
	adb push $(EXE_NAME) /data/local/tmp
	adb shell /data/local/tmp/$(EXE_NAME) 

host-sched:
	$(eval ARCH = host)
	$(eval SCHED = auto_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 
	@echo
	@echo ./$(EXE_NAME)
	@LD_LIBRARY_PATH=$(HALIDE_ROOT)/lib \
						 ./$(EXE_NAME)

host-nosched:
	$(eval ARCH = host)
	$(eval SCHED = manual_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 
	@echo
	@echo ./$(EXE_NAME)
	@LD_LIBRARY_PATH=$(HALIDE_ROOT)/lib \
						 ./$(EXE_NAME)

host-gpu:
	$(eval ARCH = host-$(OPENGL_TARGET))
	$(eval SCHED = manual_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 
	@echo
	@echo ./$(EXE_NAME)
	@LD_LIBRARY_PATH=$(HALIDE_ROOT)/lib\
						 ./$(EXE_NAME)

arm64-auto-sched:
	$(eval ARCH = arm-64-android)
	$(eval SCHED = auto_sched)
	$(eval SCHED_PARAMS = machine_params=4,1048576,10)
	@make --no-print-directory \
		ARCH=$(ARCH) SCHED=$(SCHED) SCHED_PARAMS=$(SCHED_PARAMS) 

arm64-manual-sched:
	$(eval ARCH = arm-64-android)
	$(eval SCHED = manual_sched)
	@make --no-print-directory ARCH=$(ARCH) SCHED=$(SCHED) 
