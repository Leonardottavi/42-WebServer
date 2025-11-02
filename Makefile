NAME = webserv
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
OBJ_FILES = $(addprefix $(OBJ_DIR)/, $(OBJ))
FLAGS = -Wall -Wextra -Werror -std=c++98
HPP = $(wildcard *.hpp)

.PHONY: all clean fclean re

$(NAME): $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJ_FILES) $(HPP) | $(BIN_DIR)
	g++ $(FLAGS) $(OBJ_FILES) -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	g++ -c $(FLAGS) $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

all: $(NAME)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean

re: fclean all
