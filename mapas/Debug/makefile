################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: mapas

dependents:
	-cd /home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander/Debug && $(MAKE) all

# Tool invocations
mapas: $(OBJS) $(USER_OBJS) /home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander/Debug/libBibliotecaCharMander.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander/Debug" -o "mapas" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) mapas
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander/Debug/libBibliotecaCharMander.so:

-include ../makefile.targets
