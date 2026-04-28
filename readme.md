# MoneyFlow

Gerenciador de finanças pessoais via CLI, desenvolvido em C com SQLite.

## Dependências
- GCC
- SQLite3
sudo apt install gcc libsqlite3-dev

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
