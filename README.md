<h1 align="center">SLOW-peripheral 🌐📦</h1>
<p align="center">Trabalho prático para a disciplina SSC0142 - Redes de Computadores</p>

<p align="center">
  <a href="#estrutura-do-projeto"> Estrutura do Projeto</a> • 
  <a href="#instalacao"> Instalação e Uso</a> • 
  <a href="#objetivo"> Objetivo</a> •
  <a href="#funcionamento"> Funcionamento </a> • 
  <a href="#licença">Licença</a> •
  <a href="#agradecimentos">Agradecimentos</a> •
  <a href="#equipe">👥 Equipe</a>
</p>

---

## <div id="estrutura-do-projeto"></div> Estrutura do Projeto

```bash
slow-peripheral/
├── include/
│   └── slow_client.hpp        # Definições da estrutura de pacote e funções auxiliares
├── src/
│   ├── main.cpp               # Entrada principal da aplicação
│   └── slow_client.cpp        # Implementação da lógica do cliente SLOW
├── Makefile                   # Script de build
└── README.md                  # Este arquivo
```


## <div id="instalacao"></div>⚙️ Instalação e Uso

### Compilação

1. Clone o repositório:
```bash
git clone https://github.com/SEU_USUARIO/slow-peripheral.git
cd slow-peripheral
```

2. Compile com o Makefile:
```bash
make
```

3. Execute:
```bash
./slow_client
```


## <div id="objetivo"></div> Objetivo
Este projeto implementa um cliente peripheral que se comunica com um servidor central por meio do protocolo SLOW, construído sobre UDP. Ele foi desenvolvido como parte da disciplina SSC0142 – Redes de Computadores.

## <div id="funcionamento"></div> Funcionamento

O cliente `slow_client` implementa o comportamento de um **peripheral** no protocolo de transporte **SLOW**, um protocolo confiável baseado em **UDP**, utilizando a porta **7033**. O protocolo simula características de protocolos como TCP, porém com controle manual de conexão, fluxo e fragmentação.

---

### 1. Estabelecimento de Conexão (3-Way Handshake)

- O peripheral inicia a conexão com o envio de um pacote com:
  - **UUID nulo** (sid = 0)
  - **flag `Connect` ativada**
- O servidor central responde com um pacote de **Setup**, que contém:
  - UUID da sessão
  - Tempo de vida da sessão (`sttl`)
  - Número de sequência inicial (`seqnum`)
- O peripheral armazena essas informações e as utiliza para os próximos pacotes da sessão.

---

### 2. Envio de Dados

Após o handshake, o peripheral pode enviar dados para o central:

- Os dados são encapsulados em pacotes `slow_packet_t` com:
  - UUID da sessão
  - `seqnum` incremental
  - Campo `acknum` com o último pacote recebido do central
- Os pacotes podem ser:
  - **Fragmentados**, se excederem **1440 bytes**. Nesse caso:
    - Todos os fragmentos têm o mesmo `fid`
    - A ordem dos fragmentos é dada por `fo` (Fragment Offset)
    - A flag `MB` (More Bits) indica se ainda há fragmentos a serem enviados
- O campo `window`, recebido do central, representa o tamanho da **janela de recepção disponível** (controle de fluxo)
  - O peripheral **deve respeitar** essa janela e só enviar pacotes se houver espaço
- Se não receber um `ACK` dentro do tempo limite, o pacote é **reenviado**

---

### 3. Reconexão Rápida (0-Way Connect)

- Se o tempo de vida da sessão (`sttl`) ainda não expirou, é possível reconectar usando a flag `Revive`
- O pacote de dados é enviado com a flag `Revive` ligada
- O central responde com um pacote indicando se a sessão foi aceita novamente ou se expirou

---

### 4. Desconexão

- A desconexão é realizada com o envio de um pacote com as flags:
  - `Connect = 1`, `Revive = 1`, `ACK = 1`
- Isso coloca a sessão no estado inativo
- O central confirma com um `ACK`
- Sessões inativas não devem mais aceitar pacotes comuns

---

### Estrutura do Pacote (slow_packet_t)

Todos os pacotes seguem a estrutura abaixo, em formato **little endian**:

| Campo      | Tamanho     | Descrição                                 |
|------------|-------------|--------------------------------------------|
| `sid`      | 128 bits    | UUIDv8 da sessão                          |
| `sttl`     | 27 bits     | Tempo de vida da sessão (TTL)             |
| `flags`    | 5 bits      | Flags de controle (Connect, Revive, etc.) |
| `seqnum`   | 32 bits     | Número de sequência                       |
| `acknum`   | 32 bits     | Último pacote recebido                    |
| `window`   | 16 bits     | Espaço disponível no buffer do receptor   |
| `fid`      | 8 bits      | Fragment ID                               |
| `fo`       | 8 bits      | Fragment Offset                           |
| `data`     | até 1440 B  | Dados do payload                          |

---

### Resumo da Lógica de Fluxo

```text
[1] Connect -------------------> central
                   <----------- Setup (Accept/Reject)
[2] Data (c/ ou s/ fragmentos) --> central
                   <----------- Ack
[3] Disconnect ----------------> central
                   <----------- Ack
```
---

### Servidor UDP em Python (Simulador Silencioso)

Para testar o mecanismo de **reenvio automático de mensagens** após o timeout da janela de envio, foi desenvolvido um servidor UDP simples em Python. Esse servidor **recebe os pacotes do cliente**, mas **não envia nenhuma resposta** (ACK). Assim, ele permite verificar se o cliente está corretamente reenviando os pacotes que permanecem no buffer por tempo excessivo.

Esse teste é fundamental para validar a robustez do protocolo de envio confiável (SLOW), garantindo que o cliente consiga detectar falhas na comunicação e tentar retransmissões automaticamente.

> O servidor imprime apenas o primeiro pacote de conexão (`CONNECT`) recebido e, em seguida, conta silenciosamente quantas vezes cada pacote com o mesmo `SEQNUM` é reenviado.

---

## <div id="licenca"></div>Licença
Este projeto está licenciado sob a Licença MIT. Veja o arquivo LICENSE para mais detalhes.

## <div id="acknowledgements"></div>Agradecimentos
Gostaríamos de agradecer ao monitor Gabriel Cruz, pela sua orientação e apoio ao longo deste projeto.

## <div id="equipe"></div>👥 Equipe
- Gabriel de Andrade Abreu - **14571362** ([Github](https://github.com/OGabrielAbreuBr))
- João Pedro Viguini T.T. Correa - **14675503** ([Github](https://github.com/MatheusPaivaa))
- Matheus Paiva Angarola - **12560982** ([Github](https://github.com/MatheusPaivaa))

Estudantes de Bacharelado em Ciência da Computação - USP
