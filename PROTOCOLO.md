## Protocolo

A comunicação entre o uC e a maquininha será simulada por uma interface serial entre o uC e o computador (programa em python), aqui a maquininha será um dispositivo passivo que só envia informações quando requisitado pelo device que a controla.

O diagrama a seguir ilustra como a comunicação deve ocorrer:

``` 
  uc            pc (maquininha)
   |    valor    |
   | ----------> |          
   |   ack       |
   | <---------  | ------> ack user
   |             | 
   |  req_status | <----- ok / fail / cancel
   | ----------> |
   |   status    |
   | <--------   |
   |             |
```

Protocolo:

| Head   | comando | Dado   | EOP    |
| ------ | ------- | ----   | ------ |
| 8 bits | 8 bits  | 8 bits | 8 bits |

#### Head

O head varia de acordo com quem está mandando a informação:

- uc -> pc : `U` (0x55)
- pc -> uc : `P` (0x50)

#### Comandos

- 0x00 - Verifica conexão
- 0x01 - Cobra valor &nbsp;
- 0x02 - Verifica pagamento

OBS: A maquininha sempre responde com 0x22 no comando.

#### EOP

Fim de pacote, sempre é o caracter `X` (0x58).

#### Comandos / Dado

Sempre o uC quem faz o requerimento (enviando um pacote), para cada requerimento existe uma resposta da maquininha, a resposta padrão da maquininha é o ack.

##### Verifica conexão

Verifica se maquininha está conectada.

Request:

| comando  | dado   |
| -------- | ------ |
| 0x00     | 0x00   |

Reply: 

| comando  | dado        |
| -------- | ------      |
| 0x22     | 0xFF : ok   |
|          | 0x00 : fail |

Exemplo:

``` 
 request: 0x55 0x00 0x00 0x58
 reply  : 0x50 0x22 0xFF 0x58   : ok
```

##### Cobra valor

Realiza uma nova cobrança, passando o valor da cobrança (reais inteiros) no dado

Request:

| comando  | dado           |
| -------- | ------         |
| 0x01     | Valor em reais |

Reply:

| comando  | dado        |
| -------- | ------      |
| 0x22     | 0xFF : ok   |
|          | 0x00 : fail |

Exemplo (cobra 15 reais):

``` 
 request: 0x55 0x01 0x0F 0x58
 reply  : 0x50 0x22 0xFF 0x58   : ok
```

##### Verifica pagamento

Verifica o status da requisição de pagamento:

- Pago
- Falhou
- Cancelado

Request:

| comando  | dado   |
| -------- | ------ |
| 0x02     | Valor  |

Reply:

| comando  | dado                        |
| -------- | ------                      |
| 0x22     | - 0x00 : Falhou             |
|          | - 0x01 : Aguardando usuário |
|          | - 0x02 : Cancelado          |
|          | - 0xFF : Pago               |

Exemplo:

``` 
 request: 0x55 0x02 0x00 0x58
 reply  : 0x50 0x22 0x01 0x58   : aguardando usuário
 
 espera por 10 segundos, usuário confirmou pagamento.
 
 request: 0x55 0x02 0x00 0x58
 reply  : 0x50 0x22 0xFF 0x58   : pago
 
 se caso timeout > 10s.
 
 request: 0x55 0x02 0x00 0x58
 reply  : 0x50 0x22 0x00 0x58   : falhou
```
