###############################################################################
## Copyright(C) 2014-2024 Qiware technology Co., Ltd
##
## 文件名: Makefile
## 版本号: 1.0
## 描  述: 日志服务模块
## 作  者: # Qifeng.zou # 2014.09.01 #
###############################################################################
include $(PROJ)/make/build.mak

INCLUDE = -I./incl \
		  -I$(PROJ)/src/incl \
		  -I$(PROJ)/../cctrl/src/incl \
		  -I$(PROJ)/src/exec/monitor/incl \
		  -I$(PROJ)/src/exec/filter/incl \
		  -I$(PROJ)/src/exec/crawler/incl
INCLUDE += $(GLOBAL_INCLUDE)
LIBS_PATH = -L$(PROJ)/lib -L$(PROJ)/../cctrl/lib
LIBS = -lpthread -lcore
LIBS += $(SHARED_LIB)

SRC_LIST = monitor.c \
			mon_conf.c \
			mon_crwl.c \
			mon_filter.c

OBJS = $(subst .c,.o, $(SRC_LIST)) 
HEADS = $(call func_get_dep_head_list, $(SRC_LIST))

TARGET = monitor

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(INCLUDE) $(LIBS_PATH) $(LIBS)
	@echo "CC $@"
	@mv $@ $(PROJ_BIN)/$@-$(VERSION)
	@rm -f $(PROJ_BIN)/$@
	@ln -s $(PROJ_BIN)/$@-$(VERSION) $(PROJ_BIN)/$@
	@echo "$@ is OK!"

$(OBJS): %.o : %.c $(HEADS)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	@echo "CC $(PWD)/$<"

clean:
	@rm -fr *.o $(PROJ_BIN)/$(TARGET)
	@rm -fr *.o $(PROJ_BIN)/$(TARGET)-$(VERSION)
	@echo "rm -fr *.o $(PROJ_BIN)/$(TARGET)"
