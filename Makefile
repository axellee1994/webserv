SRC_DIR      := ./src
OBJ_DIR      := ./obj
INCLUDES_DIR := ./includes
DEP_DIR      := ./dep

OBJ_SUBDIRS := $(OBJ_DIR)/server $(OBJ_DIR)/errors $(OBJ_DIR)/request $(OBJ_DIR)/response $(OBJ_DIR)/cgi $(OBJ_DIR)/config
DEP_SUBDIRS := $(DEP_DIR)/server $(DEP_DIR)/errors $(DEP_DIR)/request $(DEP_DIR)/response $(DEP_DIR)/cgi $(DEP_DIR)/config

CXX = clang++-12
CXXFLAGS = -g -Wall -Werror -Wextra -std=c++98 -Wshadow -MMD -MP
IFLAGS   := -I$(INCLUDES_DIR)

RED = \033[1;31m
GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
RESET = \033[0m

NAME := webserv

SRC_FILES := main.cpp \
				cgi/CGI.cpp \
				request/Request.cpp \
				request/RequestParser.cpp \
				request/RequestUtils.cpp \
				response/Response.cpp \
				response/ResponseParser.cpp \
				response/ResponseUtils.cpp \
				server/ClientSocket.cpp \
				server/Multiplexer.cpp \
				server/Server.cpp \
				server/SocketUtils.cpp \
				config/ConfigParser.cpp \
				config/LocationBlock.cpp \
				config/HelperFunctions.cpp \
				config/DuplicateChecker.cpp \

SRCS      := $(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_FILES := $(SRC_FILES:.cpp=.o)
OBJS      := $(addprefix $(OBJ_DIR)/, $(OBJ_FILES))

DEP_FILES := $(SRC_FILES:.cpp=.d)
DEPS      := $(addprefix $(DEP_DIR)/, $(DEP_FILES))

INCLUDES_FILES := cgi/CGI.hpp \
					errors/Errors.hpp \
					request/Request.hpp \
					response/Response.hpp \
					server/ClientSocket.hpp \
					server/Multiplexer.hpp \
					server/Server.hpp \
					server/SocketUtils.hpp \
					config/ConfigParser.hpp \
				
INCLUDES       := $(addprefix $(INCLUDES_DIR)/, $(INCLUDES_FILES))

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(BLUE)Linking objects into executable $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(IFLAGS) -o $(NAME) $(OBJS)
	@echo "$(GREEN)--- Executable File Build Complete ---$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCLUDES) | $(OBJ_DIR) $(DEP_DIR) $(OBJ_SUBDIRS) $(DEP_SUBDIRS)
	@$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@ -MF $(DEP_DIR)/$*.d
	@echo "$(YELLOW)Compiling $<...$(RESET)"

$(OBJ_DIR) $(DEP_DIR) $(OBJ_SUBDIRS) $(DEP_SUBDIRS):
	@echo "$(BLUE)Creating directory $@...$(RESET)"
	@mkdir -p $@

-include $(DEPS)

clean:
	@$(RM) -r $(OBJ_DIR) $(DEP_DIR)
	@echo "$(YELLOW)--- All object files removed ---$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(YELLOW)--- All Library Files (.a) Removed! ---$(RESET)"
	@echo "$(YELLOW)--- Executable File Removed! ---$(RESET)"

re: fclean all
	@echo "$(BLUE)--- Rebuilding Project Completed! ---$(RESET)"

valgrind-check: $(NAME)
	@echo "$(BLUE)--- Running Valgrind Check ---$(RESET)"
	@valgrind --show-leak-kinds=all --leak-check=full --trace-children=yes -s --track-origins=yes --track-fds=yes ./webserv config4.0.conf
	@echo "$(GREEN)--- Valgrind Check Completed! ---$(RESET)"

.PHONY: all clean fclean re valgrind-check
