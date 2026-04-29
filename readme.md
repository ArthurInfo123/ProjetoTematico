# MoneyFlow

Gerenciador de finanças pessoais via CLI, desenvolvido em C com SQLite.

## Dependências
- GCC
- SQLite3

WSL2 (Ubuntu)
bashsudo apt update
sudo apt install gcc make libsqlite3-dev

macOS (Homebrew)
bashbrew install sqlite
xcode-select --install

Testar
gcc --version
sqlite3 --version



## Compilação
```bash
make
```

## Execução
```bash
make run
# ou
./moneyflow
```

## Limpeza
```bash
make clean
```

## Estrutura
```
moneyflow/
├── src/          # Código-fonte .c
├── include/      # Headers .h
├── db/           # schema.sql e arquivo .db (gerado em runtime)
├── docs/         # Documentação
├── Makefile
└── README.md
```

## Banco de dados
O arquivo `db/moneyflow.db` é criado automaticamente na primeira execução.
O schema completo está em `db/schema.sql` para referência.
