################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Pokemon/pokemon.c 

OBJS += \
./src/Pokemon/pokemon.o 

C_DEPS += \
./src/Pokemon/pokemon.d 


# Each subdirectory must supply rules for building sources it contributes
src/Pokemon/%.o: ../src/Pokemon/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


