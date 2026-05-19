# MoneyFlow

Gerenciador de finanças pessoais via interface CLI, desenvolvido em **C** com persistência em **SQLite**.

Projeto acadêmico — Engenharia de Software, UNIFACEF.

---

## Sumário

- [Dependências](#dependências)
- [Compilação](#compilação)
- [Execução](#execução)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Banco de dados](#banco-de-dados)
- [Guia de uso do CLI](#guia-de-uso-do-cli)
- [Segurança](#segurança)

---

## Dependências

| Ferramenta | Versão mínima |
|---|---|
| GCC | 9.0+ |
| Make | 3.8+ |
| SQLite3 | 3.31+ |

### Instalação das dependências

**macOS**
```bash
xcode-select --install
brew install sqlite
```

**Linux / WSL2 (Ubuntu)**
```bash
sudo apt update
sudo apt install gcc make libsqlite3-dev
```

**Windows**

Recomendado usar o **WSL2** com Ubuntu. Após instalar o WSL2:
```bash
# No PowerShell como administrador
wsl --install

# Após reiniciar, dentro do Ubuntu
sudo apt update && sudo apt install gcc make libsqlite3-dev git
```

### Verificar instalação
```bash
gcc --version
sqlite3 --version
make --version
```

---

## Compilação

```bash
git clone https://github.com/SEU_USUARIO/ProjetoTematico.git
cd ProjetoTematico
make
```

---

## Execução

```bash
make run
# ou
./moneyflow
```

### Outros comandos

```bash
make clean        # remove binário e arquivos objeto
make resetdb      # apaga o banco de dados (será recriado no próximo run)
```

---

## Estrutura do projeto

```
ProjetoTematico/
├── src/                  # Código-fonte (.c)
│   ├── main.c            # Ponto de entrada e menus
│   ├── auth.c            # Cadastro e login de usuários
│   ├── sha256.c          # Hash SHA-256 (implementação standalone)
│   ├── db.c              # Conexão e inicialização do banco
│   ├── session.c         # Controle de sessão do usuário logado
│   ├── ui.c              # Helpers de interface CLI
│   ├── Validation.c      # Validação centralizada de dados
│   ├── Accounts.c        # CRUD de contas financeiras
│   ├── Categories.c      # CRUD de categorias
│   ├── Transactions.c    # Movimentações e transferências
│   ├── Bills.c           # Contas a pagar
│   └── Dashboard.c       # Visão geral consolidada
├── include/              # Headers (.h)
├── db/
│   └── schema.sql        # Estrutura das tabelas (dump do banco)
├── docs/                 # Documentação adicional
├── Makefile
└── README.md
```

> O arquivo `db/moneyflow.db` é gerado automaticamente na primeira execução e **não é versionado**.

---

## Banco de dados

O sistema utiliza **SQLite 3**. O banco é criado automaticamente no boot via `db_init()`.

A estrutura completa está em `db/schema.sql`. Tabelas:

| Tabela | Descrição |
|---|---|
| `users` | Usuários cadastrados |
| `accounts` | Contas financeiras (carteira, banco, etc.) |
| `categories` | Categorias de movimentação |
| `transactions` | Receitas, despesas e transferências |
| `bills` | Contas a pagar mensais |
| `goals` | Metas de economia |
| `credit_cards` | Cartões de crédito |
| `loans` | Empréstimos |

Para inspecionar o banco via terminal:
```bash
sqlite3 db/moneyflow.db
```
```sql
.headers on
.mode column
SELECT * FROM users;
.quit
```

---

## Guia de uso do CLI

A navegação é feita exclusivamente por **opções numéricas**. Todas as telas seguem o padrão:

```
----------------------------------------
  Nome da Tela
----------------------------------------
  1. Opcao A
  2. Opcao B
  0. Voltar / Sair
----------------------------------------
Opcao:
```

### Menu inicial

```
1. Login          → acessa o sistema com usuário e senha cadastrados
2. Cadastrar conta → cria um novo usuário
0. Sair
```

### Menu principal (após login)

```
1. Visao geral       → dashboard com saldo, contas a pagar e movimentações do mês
2. Contas            → gerenciar contas financeiras
3. Categorias        → gerenciar categorias de movimentação
4. Movimentacoes     → registrar receitas, despesas e transferências
7. Contas a pagar    → gerenciar e baixar contas mensais
0. Logout
```

---

### Fluxo 1 — Primeiro acesso

```
Menu inicial → 2. Cadastrar conta
  Nome de usuario: joao
  Senha: ••••••
  Confirme a senha: ••••••
  [OK] Usuario cadastrado com sucesso!

Menu inicial → 1. Login
  Usuario: joao
  Senha: ••••••
  [OK] Login realizado com sucesso!
```

---

### Fluxo 2 — Configuração inicial

Antes de registrar movimentações, crie pelo menos uma **conta** e uma **categoria**:

```
Menu principal → 2. Contas → 2. Adicionar conta
  Nome da conta: Carteira
  Saldo inicial: R$ 500.00
  [OK] Conta adicionada com sucesso!

Menu principal → 3. Categorias → 2. Adicionar categoria
  Nome da nova categoria: Alimentacao
  Categoria adicionada com sucesso!
```

---

### Fluxo 3 — Registrar movimentação

```
Menu principal → 4. Movimentacoes → 1. Registrar receita ou despesa
  1. Receita
  2. Despesa
  Tipo: 2
  Valor: R$ 45.90
  Data (DD/MM/AAAA): 19/05/2026
  Descricao: Supermercado
  [lista de contas]
  ID da conta: 1
  [lista de categorias]
  ID da categoria: 1
  [OK] Movimentacao registrada com sucesso!
```

---

### Fluxo 4 — Baixar conta a pagar

```
Menu principal → 7. Contas a pagar → 5. Baixar conta
  [lista de contas pendentes]
  ID da conta a baixar: 1
  [lista de contas financeiras]
  ID da conta financeira para debito: 1
  Confirmar baixa?
    Conta a pagar : Luz
    Valor         : R$ 120.00
    Debitar de    : conta ID 1
  1. Confirmar  0. Cancelar
  Opcao: 1
  [OK] Conta baixada com sucesso! Despesa registrada automaticamente.
```

---

## Segurança

- Senhas armazenadas com **hash SHA-256** — nunca em texto puro
- Sessão mantida em memória via `struct Session` — encerrada ao sair
- Todas as queries filtradas por `user_id` — isolamento completo entre usuários
- Nenhum dado de outro usuário é acessível após o login