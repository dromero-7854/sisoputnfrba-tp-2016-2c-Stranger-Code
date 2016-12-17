################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Entrenador/entrenador.c 

OBJS += \
./src/Entrenador/entrenador.o 

C_DEPS += \
./src/Entrenador/entrenador.d 


# Each subdirectory must supply rules for building sources it contributes
src/Entrenador/%.o: ../src/Entrenador/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


