CC = gcc
CXX = g++
CPPFLAGS = -std=c++0x -O2 -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer
DEBUG_CXXFLAGS = -g
SRCEXTS = .c .cc .cpp .proto

MUDUP_ROOT_PATH = ./third_party/muduo
MUDUO_INC_PATH = ./third_party/muduo
MUDUO_LIB_PATH = ./third_party/muduo/lib

HDRPATHS = -I./src -I$(MUDUO_INC_PATH)
LIBPATHS = -L./src -L$(MUDUO_LIB_PATH)

LR_DIRS = ./src
LR_SOURCES = $(foreach d,$(LR_DIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
LR_OBJS = $(addsuffix .o, $(basename $(LR_SOURCES)))

LINK_OPTIONS = -Xlinker -lmuduo_base -lpthread -ldl -lz -lrt

.PHONY:all
all: log_receiver

.PHONY:log_receiver
log_receiver: $(LR_OBJS) third_party_lib
	@echo "Linking $@"
	$(CXX) $(LR_OBJS) -o $@ $(LIBPATHS) $(LINK_OPTIONS)

.PHONY:third_party_lib
third_party_lib: muduo_lib

.PHONY:muduo_lib
muduo_lib:
	cmake -DCMAKE_BUILD_NO_EXAMPLES=1 $(MUDUP_ROOT_PATH)
	make -C $(MUDUP_ROOT_PATH)

.PHONY:clean
clean:
	@echo "Cleaning"
	@rm -rf ./src/*.o ./log_receiver


%.o:%.cpp
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(CXXFLAGS) $< -o $@

%.dbg.o:%.cpp
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(DEBUG_CXXFLAGS) $< -o $@

%.o:%.cc
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(CXXFLAGS) $< -o $@

%.dbg.o:%.cc
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(DEBUG_CXXFLAGS) $< -o $@

%.o:%.mm
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(CXXFLAGS) $< -o $@

%.dbg.o:%.mm
	@echo "Compiling $@"
	@$(CXX) -c $(HDRPATHS) $(DEBUG_CXXFLAGS) $< -o $@

%.o:%.c
	@echo "Compiling $@"
	@$(CC) -c $(HDRPATHS) $(CFLAGS) $< -o $@

%.dbg.o:%.c
	@echo "Compiling $@"
	@$(CC) -c $(HDRPATHS) $(DEBUG_CFLAGS) $< -o $@

