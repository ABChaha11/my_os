# 定义编译器和工具链前缀
CC = riscv64-unknown-elf-gcc
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

# 定义编译和链接选项
# -nostdlib: 不链接标准库
# -fno-builtin: 不使用内置函数
# -mcmodel=medany: 使用中等代码模型，适用于内核
# -Wall: 开启所有警告
# -g: 生成调试信息
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany -Wall -g -Iinclude
# -T kernel/kernel.ld: 使用我们的链接脚本
LDFLAGS = -T kernel/kernel.ld

# --- 修改开始 ---
# 定义所有的源文件
# 新增了 kernel/pmm.c 和 kernel/vm.c
SRCS = kernel/entry.S kernel/main.c kernel/uart.c kernel/console.c kernel/printf.c kernel/pmm.c kernel/vm.c
# --- 修改结束 ---

# 根据源文件自动生成目标文件列表 (.o)
OBJS = $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS)))

# 最终生成的目标文件名
TARGET = kernel/kernel.elf

# 默认目标，第一个目标是 `make` 命令的默认执行目标
all: $(TARGET)

# 链接规则：如何从所有 .o 文件生成最终的 .elf 文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 汇编规则：如何从 .S 文件生成 .o 文件
kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@

# C编译规则：如何从 .c 文件生成 .o 文件
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理命令：删除所有生成的文件
clean:
	rm -f $(TARGET) $(OBJS)

# QEMU 运行命令
# -machine virt: 使用 QEMU 的 virt 虚拟平台
# -nographic: 不使用图形界面，所有输出重定向到终端
# -bios none: 不加载默认的 BIOS/Firmware
# -kernel $(TARGET): 将我们的内核作为可执行文件加载
# -m 128M: 设置内存为128M
qemu: $(TARGET)
	qemu-system-riscv64 -machine virt -bios none -kernel $(TARGET) -nographic -m 128M

# QEMU 调试命令
# -S: 启动后冻结 CPU，等待 GDB 连接
# -s: 在 1234 端口开启 GDB server (这是 -gdb tcp::1234 的简写)
qemu-gdb: $(TARGET)
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel $(TARGET) -S -s -m 128M

# 声明 clean 和 qemu* 不是文件名
.PHONY: all clean qemu qemu-gdb