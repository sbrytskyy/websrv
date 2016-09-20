-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
OBJ_SRCS := 
ASM_SRCS := 
C_SRCS := 
O_SRCS := 
S_UPPER_SRCS := 
EXECUTABLES := 
OBJS := 
C_DEPS := 

# Every subdirectory with source files must be described here
SUBDIRS := \
src \


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
src/protocol.c \
src/server.c \
src/storage.c \
src/utils.c \
src/websrv.c \
src/worker.c 

OBJS += \
./src/protocol.o \
./src/server.o \
./src/storage.o \
./src/utils.o \
./src/websrv.o \
./src/worker.o 

C_DEPS += \
./src/protocol.d \
./src/server.d \
./src/storage.d \
./src/utils.d \
./src/websrv.d \
./src/worker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
# debug
#	gcc -DDEBUG -I"include" -O0 -g3 -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
# release
	gcc -I"include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



USER_OBJS :=

LIBS := -lpthread -laxtls


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: websrv

# Tool invocations
websrv: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"lib" -o "websrv" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) websrv
	-@echo ' '

.PHONY: all clean dependents

