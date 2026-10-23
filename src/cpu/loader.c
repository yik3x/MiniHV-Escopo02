#include "loader.h"
#include "../util/util.h"

static int parse_register(const char *token) {
    if (token == NULL || (token[0] != 'R' && token[0] != 'r') ||
        token[1] == '\0' || token[2] != '\0') return -1;
    if (token[1] < '0' || token[1] > '3') return -1;
    return token[1] - '0';
}

static bool parse_number(const char *token, int32_t *value) {
    return util_parse_i32(token, value);
}

static bool no_extra_token(void) {
    return strtok(NULL, " ,\t\r\n[]") == NULL;
}

static bool parse_instruction(char *line, Instruction *instruction) {
    char *op_token;
    char *a;
    char *b;
    char *c;
    int reg_a;
    int reg_b;
    int32_t number;
    memset(instruction, 0, sizeof(*instruction));
    instruction->op = OP_INVALID;
    op_token = strtok(line, " ,\t\r\n[]");
    if (op_token == NULL) return false;
    for (char *p = op_token; *p != '\0'; ++p) *p = (char)toupper((unsigned char)*p);

    if (strcmp(op_token, "HALT") == 0) {
        instruction->op = OP_HALT;
        return no_extra_token();
    }
    if (strcmp(op_token, "MOV") == 0 || strcmp(op_token, "ADDI") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); b = strtok(NULL, " ,\t\r\n[]");
        reg_a = parse_register(a);
        if (reg_a < 0 || !parse_number(b, &number) || !no_extra_token()) return false;
        instruction->rd = reg_a; instruction->imm = number;
        instruction->op = strcmp(op_token, "MOV") == 0 ? OP_MOV : OP_ADDI;
        return true;
    }
    if (strcmp(op_token, "ADD") == 0 || strcmp(op_token, "SUB") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); b = strtok(NULL, " ,\t\r\n[]");
        reg_a = parse_register(a); reg_b = parse_register(b);
        if (reg_a < 0 || reg_b < 0 || !no_extra_token()) return false;
        instruction->rd = reg_a; instruction->rs = reg_b;
        instruction->op = strcmp(op_token, "ADD") == 0 ? OP_ADD : OP_SUB;
        return true;
    }
    if (strcmp(op_token, "LOAD") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); b = strtok(NULL, " ,\t\r\n[]");
        reg_a = parse_register(a);
        if (reg_a < 0 || b == NULL || !no_extra_token()) return false;
        instruction->op = OP_LOAD; instruction->rd = reg_a;
        reg_b = parse_register(b);
        if (reg_b >= 0) { instruction->indirect = true; instruction->rs = reg_b; }
        else if (!parse_number(b, &instruction->imm)) return false;
        return true;
    }
    if (strcmp(op_token, "STORE") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); b = strtok(NULL, " ,\t\r\n[]");
        reg_a = parse_register(a);
        if (reg_a < 0 || b == NULL || !no_extra_token()) return false;
        instruction->op = OP_STORE; instruction->rs = reg_a;
        reg_b = parse_register(b);
        if (reg_b >= 0) { instruction->indirect = true; instruction->rd = reg_b; }
        else if (!parse_number(b, &instruction->imm)) return false;
        return true;
    }
    if (strcmp(op_token, "JMP") == 0) {
        a = strtok(NULL, " ,\t\r\n[]");
        if (!parse_number(a, &number) || !no_extra_token()) return false;
        instruction->op = OP_JMP; instruction->target = number;
        return true;
    }
    if (strcmp(op_token, "JZ") == 0 || strcmp(op_token, "JNZ") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); b = strtok(NULL, " ,\t\r\n[]");
        reg_a = parse_register(a);
        if (reg_a < 0 || !parse_number(b, &number) || !no_extra_token()) return false;
        instruction->op = strcmp(op_token, "JZ") == 0 ? OP_JZ : OP_JNZ;
        instruction->rs = reg_a; instruction->target = number;
        return true;
    }
    if (strcmp(op_token, "OUT") == 0 || strcmp(op_token, "DISK") == 0) {
        a = strtok(NULL, " ,\t\r\n[]"); reg_a = parse_register(a);
        if (reg_a < 0 || !no_extra_token()) return false;
        instruction->op = strcmp(op_token, "OUT") == 0 ? OP_OUT : OP_DISK;
        instruction->rs = reg_a;
        return true;
    }
    if (strcmp(op_token, "NET") == 0) {
        a = strtok(NULL, " ,\t\r\n[]");
        if (!parse_number(a, &number) || number < 0 || !no_extra_token()) return false;
        instruction->op = OP_NET; instruction->imm = number;
        return true;
    }
    (void)c;
    return false;
}

bool cpu_load_program(Vm *vm, const char *path, char *error, size_t error_capacity) {
    FILE *file;
    char line_buffer[MAX_LINE_LEN];
    size_t line_number = 0U;
    size_t capacity = 32U;
    Instruction *instructions;
    if (vm == NULL || path == NULL) return false;
    file = fopen(path, "r");
    if (file == NULL) {
        (void)snprintf(error, error_capacity, "%s: nao foi possivel abrir o programa", path);
        return false;
    }
    instructions = (Instruction *)malloc(capacity * sizeof(*instructions));
    if (instructions == NULL) { (void)fclose(file); (void)snprintf(error, error_capacity, "%s: memoria insuficiente", path); return false; }
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        char *comment;
        Instruction parsed;
        line_number++;
        comment = strchr(line_buffer, ';');
        if (comment != NULL) *comment = '\0';
        util_trim(line_buffer);
        if (line_buffer[0] == '\0') continue;
        if (!parse_instruction(line_buffer, &parsed)) {
            (void)snprintf(error, error_capacity, "%s:%lu: instrucao MV-ISA invalida: %s",
                           path, (unsigned long)line_number, line_buffer);
            free(instructions); (void)fclose(file); return false;
        }
        if (vm->program_count >= MAX_INSTRUCTIONS) {
            (void)snprintf(error, error_capacity, "%s:%lu: maximo de %d instrucoes excedido",
                           path, (unsigned long)line_number, MAX_INSTRUCTIONS);
            free(instructions); (void)fclose(file); return false;
        }
        if (vm->program_count == capacity) {
            size_t new_capacity = capacity * 2U;
            Instruction *new_items;
            if (new_capacity > MAX_INSTRUCTIONS) new_capacity = MAX_INSTRUCTIONS;
            new_items = (Instruction *)realloc(instructions, new_capacity * sizeof(*instructions));
            if (new_items == NULL) {
                (void)snprintf(error, error_capacity, "%s:%lu: memoria insuficiente",
                               path, (unsigned long)line_number);
                free(instructions); (void)fclose(file); return false;
            }
            instructions = new_items; capacity = new_capacity;
        }
        instructions[vm->program_count++] = parsed;
    }
    if (ferror(file)) {
        (void)snprintf(error, error_capacity, "%s: erro de leitura", path);
        free(instructions); (void)fclose(file); return false;
    }
    (void)fclose(file);
    if (vm->program_count == 0U) {
        (void)snprintf(error, error_capacity, "%s: programa vazio", path);
        free(instructions); return false;
    }
    if (vm->mem_words < vm->program_count) {
        (void)snprintf(error, error_capacity, "%s: mem_words e menor que o programa", path);
        free(instructions); return false;
    }
    vm->program = instructions;
    (void)snprintf(vm->program_path, sizeof(vm->program_path), "%s", path);
    return true;
}
