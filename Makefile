CC = gcc
CXX = g++
CPPFLAGS = -D__const__= -D_GNU_SOURCE -DUSE_SYMBOLIZE
CXXFLAGS = $(CPPFLAGS) -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-invalid-offsetof -Wno-unused-parameter -fno-omit-frame-pointer
CFLAGS = $(CPPFLAGS) -O2 -pipe -Wall -W -fPIC -fstrict-aliasing -Wno-unused-parameter -fno-omit-frame-pointer
DEBUG_CXXFLAGS = $(filter-out -DNDEBUG,$(CXXFLAGS)) -DUNIT_TEST
DEBUG_CFLAGS = $(filter-out -DNDEBUG,$(CFLAGS)) -DUNIT_TEST
SRCEXTS = .c .cc .cpp .proto

MUDUP_ROOT_PATH = ./third_party/muduo
MUDUO_INC_PATH = ./third_party/muduo
MUDUO_LIB_PATH = ./third_party/muduo/lib

HDRPATHS = -I./src -I$(MUDUO_INC_PATH)
LIBPATHS = -L./src -L$(MUDUO_LIB_PATH)

LR_DIRS = ./src
LR_SOURCES = $(foreach d,$(LR_DIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
LR_OBJS = $(addsuffix .o, $(basename $(LR_SOURCES)))

TEST_DIRS = ./tests
TEST_SOURCES = $(foreach d,$(TEST_DIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
TEST_OBJS = $(addsuffix .o, $(basename $(TEST_SOURCES)))

LINK_OPTIONS = -Xlinker -lmuduo_base -lpthread -ldl -lz -lrt

.PHONY:all
all: log_receiver log_client

.PHONY:log_receiver
log_receiver: $(LR_OBJS) third_party_lib
	@echo "Linking $@"
	$(CXX) $(LR_OBJS) -o $@ $(LIBPATHS) $(LINK_OPTIONS)

.PHONY:log_client
log_client: $(TEST_OBJS) third_party_lib
	@echo "Linking $@"
	$(CXX) $(TEST_OBJS) -o $@ $(LIBPATHS) $(LINK_OPTIONS)

.PHONY:third_party_lib
third_party_lib: muduo_lib

.PHONY:muduo_lib
muduo_lib:
	cd $(MUDUP_ROOT_PATH) && cmake -DCMAKE_BUILD_NO_EXAMPLES=1 . && make

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

