<p align="center">
  <img src="https://cdn0.sbnation.com/assets/3417793/moveoverlynnswann.gif"/>
</p>

# UDP Broadcast

## Tópicos
* [Introdução](#introdução)
* [Endereço Broadcast](#endereço-broadcast)
* [Identificando o endereço de broadcast](#identificando-o-endereço-de-broadcast)
* [Preparação do Ambiente](#preparação-do-ambiente)
* [netcat](#netcat)
* [tcpdump](#tcpdump)
* [Implementação](#implementação)
* [Biblioteca](#biblioteca)
* [udp_broadcast_receiver.h](#udp_broadcast_receiverh)
* [udp_broadcast_receiver.c](#udp_broadcast_receiverc)
* [udp_broadcast_sender.h](#udp_broadcast_senderh)
* [udp_broadcast_sender.c](#udp_broadcast_senderc)
* [launch_processes](#launch_processes)
* [button_interface](#button_interface)
* [led_interface](#led_interface)
* [button_process](#button_process)
* [led_process](#led_process)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Modo PC](#modo-pc)
* [Modo RASPBERRY](#modo-raspberry)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [Monitorando o tráfego usando o tcpdump](#monitorando-o-tráfego-usando-o-tcpdump)
* [Testando conexão com o servidor via netcat](#testando-conexão-com-o-servidor-via-netcat)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referência](#referência)

## Introdução
O UDP no modo broadcast permite enviar mensagens para todas as máquinas conectadas na rede de uma única vez, para exemplificar tome o a televisão como exemplo, o sinal de TV é transmitido pelo ar onde qualquer televisão sintonizada nessa determinada frequência pode captar o programa transmitido, esse tipo de aplicação seria inviável se o sinal fosse enviado para cada televisor existente. O broadcast está presente somente no protocolo IPv4, no IPv6 é usado uma outra técnica conhecida como multicast. O broadcast é usado no protocolo ARP(Address Resolution Protocol) que mapeia o endereço físico, o endereço MAC.

## Endereço Broadcast 
Para entender o endereço broadcast é adotado ip da classe C onde o primeiro octeto tem um range de 192 até 223, normalmente as redes domésticas utilizam essa classe como por exemplo 192.168.0.XXX. Na classe C quando a rede é descrita na forma 192.168.0.XXX, não devemos usar os valores 0 e 255, onde 0 representa a rede e o 255 representa o endereço broadcast dessa rede, sendo assim se uma mensagem for enviada para esse endereço todas as máquinas conectadas nessa rede irá receber a mensagem.


## Representação do Broadcast na rede
Quando uma mensagem é enviada para todas as máquinas irão receber para ilustrar, o exemplo representa uma mensagem broadcast enviada em uma rede classe B

<p align="center">
<img src="https://i.stack.imgur.com/Jq190.gif">
</p>

Na imagem é possível notar que as mensagem não é propagada pelo roteador

## Identificando o endereço de broadcast
Para identicar qual o endereço broadcast da rede pode ser usado o comando ip
```bash
$ ip a
```

Nas interfaces do computador é possível notar o ip que é atribuído para a máquina e em seguida o endereço de broadcast logo a frente do acrônimo brd(broadcast), esse é o endereço que é usado para enviar mensagens broadcast
```bash 
....
2: enp0s31f6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 10:65:30:22:8a:1a brd ff:ff:ff:ff:ff:ff
    inet 192.168.0.140/24 brd 192.168.0.255 scope global dynamic noprefixroute enp0s31f6
       valid_lft 4736sec preferred_lft 4736sec
....
```

<font color="red">Obs:</font> Durante o exemplo é necessário inserir esse endereço correspondente a rede em que está rodando o exemplo no descritor usado em *button_process* 

## Preparação do Ambiente
Antes de apresentarmos o exemplo, primeiro precisaremos instalar algumas ferramentas para auxiliar na análise da comunicação. As ferramentas necessárias para esse artigo são o tcpdump e o netcat(nc), para instalá-las basta executar os comandos abaixo:

```c
sudo apt-get update
```
```c
sudo apt-get install netcat
```
```c
sudo apt-get install tcpdump
```

## netcat 
O netcat é uma ferramenta capaz de interagir com conexões UDP e TCP, podendo abrir conexões, ouvindo como um servidor, ou com cliente enviando mensagens para um servidor.

## tcpdump
O tcpdump é uma ferramenta capaz de monitorar o tráfego de dados em uma dada interface como por exemplo eth0, com ele é possível analisar os pacotes que são recebido e enviados.

## Implementação

Para demonstrar o uso desse IPC, iremos utilizar o modelo Cliente/Servidor, onde o processo Cliente(_button_process_) vai enviar uma mensagem via broadcast para o Servidor(_led_process_) vai ler a mensagem, e verificar se corresponde com os comandos cadastrados internamente e aplicar o comando caso seja válido. 

## Biblioteca
A biblioteca criada permite uma fácil criação do servidor, sendo o servidor orientado a eventos, ou seja, fica aguardando as mensagens chegarem.

### udp_broadcast_receiver.h
Primeiramente criamos um callback responsável por eventos de recebimento, essa função será chamada quando houver esse evento.
```c
typedef void (*Event)(const char *buffer, size_t buffer_size, void *data);
```

Criamos também um contexto que armazena os parâmetros utilizados pelo servidor, sendo o socket para armazenar a instância criada, port que recebe o número que corresponde onde o serviço será disponibilizado, buffer que aponta para a memória alocada previamente pelo usuário, buffer_size o representa o tamanho do buffer e o callback para recepção da mensagem

```c
typedef struct 
{
    int socket;
    int port;
    char *buffer;
    size_t buffer_size;
    Event on_receive_message;
} UDP_Server;
```

Essa função inicializa o servidor com os parâmetros do contexto
```c
bool UDP_Broadcast_Receiver_Init(UDP_Receiver *receiver);
```
Essa função aguarda uma mensagem enviada pelo cliente.
```c
bool UDP_Broadcast_Receiver_Run(UDP_Receiver *receiver, void *user_data);
```

### udp_broadcast_receiver.c

No UDP_Broadcast_Receiver_Init definimos algumas variáveis para auxiliar na inicialização do servidor, sendo uma variável booleana que representa o estado da inicialização do servidor, uma variável do tipo inteiro para habilitar o reuso da porta caso o servidor precise reiniciar e uma estrutura sockaddr_in que é usada para configurar o servidor para se comunicar através da rede.

```c
bool status = false;
struct sockaddr_in server_addr;
int yes = 1;
```

Para realizar a inicialização é criado um dummy do while, para que quando houver falha em qualquer uma das etapas, irá sair da função com status de erro, nesse ponto verificamos se o contexto, o buffer e se o tamanho do buffer foi inicializado, sendo sua inicialização de responsabilidade do usuário

```c
if(!receiver || !receiver->buffer || !receiver->buffer_size)
    break;
```
Criamos um endpoint com o perfil de se conectar via protocolo IPv4(AF_INET), do tipo datagram que caracteriza o UDP(SOCK_DGRAM), o último parâmetro pode ser 0 nesse caso.
```c
receiver->socket = socket(AF_INET, SOCK_DGRAM, 0);
if(receiver->socket < 0)
    break;
```
Preenchemos a estrutura com parâmetros fornecidos pelo usuário como em qual porta que o serviço vai rodar.
```c
memset(&server_addr, 0, sizeof(server_addr));

server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;
server_addr.sin_port = htons(receiver->port);
```

Aqui permitimos o reuso do socket caso necessite reiniciar o serviço
```c
if (setsockopt(receiver->socket, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0)
    break;
```
Aplicamos as configurações ao socket criado e atribuimos true na variável status
```c
if (bind(receiver->socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    break;
status = true;
```

Na função UDP_Broadcast_Receiver_Run declaramos algumas variáveis para receber as mensagens por meio do broadcast

```c
bool status = false;
struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
size_t read_size;
```
Verificamos se o socket é válido e aguardamos uma mensagem do cliente, repassamos a mensagem para o callback realizar o tratamento de acordo com a aplicação do cliente, e retornamos o estado.
```c
if(receiver->socket > 0)
{
    read_size = recvfrom(receiver->socket, receiver->buffer, receiver->buffer_size, MSG_WAITALL,
                                (struct sockaddr *)&client_addr, &len); 
    receiver->buffer[read_size] = 0;
    receiver->on_receive_message(receiver->buffer, read_size, user_data);
    memset(receiver->buffer, 0, receiver->buffer_size);
    status = true;
}

return status;
```

### udp_broadcast_sender.h
Criamos também um contexto que armazena os parâmetros utilizados pelo cliente, sendo o socket para armazenar a instância criada, hostname é o ip que da máquina que vai enviar as mensagens e o port que recebe o número que corresponde qual o serviço deseja consumir

```c
typedef struct 
{
    int socket;
    const char *hostname;
    const char *port;
} UDP_Sender;
```

Inicializa o cliente com os parâmetros do descritor 
```c
bool UDP_Broadcast_Sender_Init(UDP_Sender *sender);
```

Envia mensagem para o servidor baseado nos parâmetros do descritor.
```c
bool UDP_Broadcast_Sender_Send(UDP_Sender *sender, const char *message, size_t message_size);
```
### udp_broadcast_sender.c

Na função UDP_Broadcast_Sender_Init verificamos se o contexto foi iniciado, configuramos o socket como UDP e habilitamos o envio no modo broadcast

```c
int broadcast_enable;
bool status = false;
do 
{
    if(!sender)
        break;

    sender->socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sender->socket < 0)
        break;

    broadcast_enable = 1;
    if(setsockopt(sender->socket, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_enable, sizeof(broadcast_enable)) < 0)
        break;

    status = true;        
}while(false);

return status;
```

Na função UDP_Broadcast_Sender_Send definimos algumas variáveis para auxiliar na comunicação com o servidor, sendo uma variável booleana que representa o estado de envio para o servidor, uma estrutura sockaddr_in que é usada para configurar o servidor no qual será enviado as mensagens e uma variável de quantidade de dados enviados.

```c
bool status = false;
struct sockaddr_in server;
ssize_t send_len;
```
Parâmetrizamos a estrutura com os dados do servidor
```c
memset(&server, 0, sizeof(server));
server.sin_family = AF_INET;
server.sin_addr.s_addr = inet_addr(sender->hostname);
server.sin_port = htons(atoi(sender->port));
```
Realizamos o envio da mensagem para o servidor
```c
send_len = sendto(sender->socket, message, message_size, 0, (struct sockaddr *)&server, sizeof(server));
  if(send_len == message_size)
      status = true;

return status;
```
Aplicação é composta por três executáveis sendo eles:
* _launch_processes_ - é responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - é responsável por ler o GPIO em modo de leitura da Raspberry Pi e escrever o estado interno no arquivo
* _led_interface_ - é responsável por ler do arquivo o estado interno do botão e aplicar em um GPIO configurado como saída

### *launch_processes*

No _main_ criamos duas variáveis para armazenar o PID do *button_process* e do *led_process*, e mais duas variáveis para armazenar o resultado caso o _exec_ venha a falhar.
```c
int pid_button, pid_led;
int button_status, led_status;
```

Em seguida criamos um processo clone, se processo clone for igual a 0, criamos um _array_ de *strings* com o nome do programa que será usado pelo _exec_, em caso o _exec_ retorne, o estado do retorno é capturado e será impresso no *stdout* e aborta a aplicação. Se o _exec_ for executado com sucesso o programa *button_process* será carregado. 
```c
pid_button = fork();

if(pid_button == 0)
{
    //start button process
    char *args[] = {"./button_process", NULL};
    button_status = execvp(args[0], args);
    printf("Error to start button process, status = %d\n", button_status);
    abort();
}   
```

O mesmo procedimento é repetido novamente, porém com a intenção de carregar o *led_process*.

```c
pid_led = fork();

if(pid_led == 0)
{
    //Start led process
    char *args[] = {"./led_process", NULL};
    led_status = execvp(args[0], args);
    printf("Error to start led process, status = %d\n", led_status);
    abort();
}
```

## *button_interface*
Definimos uma lista de comandos que iremos enviar
```c
const char *led_commands[] = 
{
    "LED ON",
    "LED OFF"
};
```
A implementação do Button_Run ficou simples, onde realizamos a inicialização do interface de botão e ficamos em loop aguardando o pressionamento do botão para alterar o estado da variável e enviar a mensagem para o servidor
```c
bool Button_Run(UDP_Sender *sender, Button_Data *button)
{
    int state = 0;

    if(button->interface->Init(button->object) == false)
        return false;

    if(UDP_Broadcast_Sender_Init(sender) == false)
        return false;

    while (true)
    {
        wait_press(button);
        state ^= 0x01;
        UDP_Broadcast_Sender_Send(sender, led_commands[state], strlen(led_commands[state]));
    }

    return false;
}
```
## *led_interface*
A implementação do LED_Run ficou simplificada, realizamos a inicialização da interface de LED, do servidor e ficamos em loop aguardando o recebimento de uma mensagem.
```c
bool LED_Run(UDP_Receiver *receiver, LED_Data *led)
{

	if(led->interface->Init(led->object) == false)
		return false;

	if(UDP_Broadcast_Receiver_Init(receiver) == false) 
		return false;


	while(true)
	{
		UDP_Broadcast_Receiver_Run(receiver, led);
	}

	return false;	
}
```

## *button_process*
A parametrização do cliente fica por conta do processo de botão que inicializa o contexto com o endereço broadcast, o serviço que deseja consumir, e assim passamos os argumentos para Button_Run iniciar o processo.

```c
UDP_Sender sender = 
{
    .hostname = "192.168.0.255",
    .port  = "1234"
};

Button_Run(&sender, &button);
```
## *led_process*
A parametrização do servidor fica por conta do processo de LED que inicializa o contexto com o buffer, seu tamanho, a porta onde vai servir e o callback preenchido, e assim passamos os argumentos para LED_Run iniciar o serviço.
```c
UDP_Server server = 
{
    .buffer = server_buffer,
    .buffer_size = BUFFER_SIZE,
    .port = 1234,
    .on_receive_message = on_receive_message
};

LED_Run(&server, &led);
```

A implementação no evento de recebimento da mensagem, compara a mensagem recebida com os comandos internos para o acionamento do LED, caso for igual executa a ação correspondente.
```c
void on_receive_message(const char *buffer, size_t buffer_size, void *data)
{
    LED_Data *led = (LED_Data *)data;

    if(strncmp("LED ON", buffer, strlen("LED ON")) == 0)
        led->interface->Set(led->object, 1);
    else if(strncmp("LED OFF", buffer, strlen("LED OFF")) == 0)
        led->interface->Set(led->object, 0);
}
```

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para facilitar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_Socket_UDP_Broadcast
$ cd Raspberry_IPC_Socket_UDP_Broadcast
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando 
```bash
$ ps -ef | grep _process
```

O output 
```bash
cssouza  30226  2298  0 16:27 pts/12   00:00:00 ./button_process
cssouza  30227  2298  0 16:27 pts/12   00:00:00 ./led_process
```
## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo
```bash
echo "0" > /tmp/broadcast_fifo
```

Output do LOG quando enviado o comando algumas vezez
```bash
May 21 16:34:26 dell-cssouza LED UDP[30227]: LED Status: On
May 21 16:34:27 dell-cssouza LED UDP[30227]: LED Status: Off
May 21 16:34:27 dell-cssouza LED UDP[30227]: LED Status: On
May 21 16:34:28 dell-cssouza LED UDP[30227]: LED Status: Off
May 21 16:34:28 dell-cssouza LED UDP[30227]: LED Status: On
May 21 16:34:29 dell-cssouza LED UDP[30227]: LED Status: Off
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## Monitorando o tráfego usando o tcpdump

Para monitorar as mensagens que trafegam, precisamos ler uma interface, para saber quais interfaces que o computador possui usamos o comando

```bash
$ ip a
```
Output
```bash
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: enp0s31f6: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 10:65:30:22:8a:1a brd ff:ff:ff:ff:ff:ff
    inet 192.168.0.140/24 brd 192.168.0.255 scope global dynamic noprefixroute enp0s31f6
       valid_lft 4736sec preferred_lft 4736sec
    inet6 2804:6828:c07d:3800:8e1:7295:eb55:6dc1/64 scope global temporary dynamic 
       valid_lft 296sec preferred_lft 296sec
    inet6 2804:6828:c07d:3800:dcee:5cbc:c056:32a2/64 scope global temporary deprecated dynamic 
       valid_lft 296sec preferred_lft 0sec
    inet6 2804:6828:c07d:3800:72d1:f865:c51c:79de/64 scope global dynamic mngtmpaddr noprefixroute 
       valid_lft 296sec preferred_lft 296sec
    inet6 fe80::3b0:2187:f4da:d8cd/64 scope link noprefixroute 
       valid_lft forever preferred_lft forever
3: wlp2s0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN group default qlen 1000
    link/ether 7c:2a:31:df:f0:02 brd ff:ff:ff:ff:ff:ff
4: docker0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc noqueue state DOWN group default 
    link/ether 02:42:15:e4:fe:cc brd ff:ff:ff:ff:ff:ff
    inet 172.17.0.1/16 brd 172.17.255.255 scope global docker0
       valid_lft forever preferred_lft forever
5: vboxnet0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 0a:00:27:00:00:00 brd ff:ff:ff:ff:ff:ff
    inet 172.16.11.100/24 brd 172.16.11.255 scope global vboxnet0
       valid_lft forever preferred_lft forever
    inet6 fe80::800:27ff:fe00:0/64 scope link 
       valid_lft forever preferred_lft forever
```
Como podemos ver temos 5 interfaces no computador onde o comando foi executado, pode ser que a máquina que esteja usando possa ter mais interfaces ou menos interfaces. Para teste local, iremos usar a interface local denominada lo, que representa a interface de loopback.

O tcpdump possui opções que permite a visualização dos dados, não irei explicar tudo, fica de estudo para quem quiser saber mais sobre a ferramenta. Executando o comando podemos ver todas as mensagens de broadcast

```bash
sudo tcpdump -i enp0s31f6 -nnSX "broadcast"
```

Após executar o comando o tcpdump ficará fazendo sniffing da interface, tudo o que for trafegado nessa interface será apresentado, dessa forma enviamos um comando e veremos a seguinte saída:
```bash
16:30:53.482390 IP 192.168.0.140.39611 > 192.168.0.255.1234: UDP, length 7
	0x0000:  4500 0023 462e 4000 4011 71c0 c0a8 008c  E..#F.@.@.q.....
	0x0010:  c0a8 00ff 9abb 04d2 000f 82fc 4c45 4420  ............LED.
	0x0020:  4f46 46                                  OFF

```
Podemos ver que não há o processo de handshake somente o envio da mensagem, como descrito a seguir:

* No instante 16:30:53.482390 IP 192.168.0.140.39611 > 192.168.0.255.1234 o cliente envia uma mensagem para o server via broadcast 

## Testando conexão com o servidor via netcat
A aplicação realiza a comunicação entre processos locais, para testar uma comunicação remota usaremos o netcat que permite se conectar de forma prática ao servidor e enviar os comandos. Para se conectar basta usar o seguinte comando:

```bash
nc -u ip port
```

Como descrito no comando ip usaremos o ip de broadcast apresentado na interface enp0s31f6 que é o IP 192.168.0.255, então o comando fica

```bash
 echo -e "LED ON" | nc -b -u  192.168.0.255 1234
```

E enviamos o comando LED ON, se visualizar no log irá apresentar que o comando foi executado, para monitorar com o tcpdump basta mudar a interface

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
O broadcast é uma boa solução para enviar mensagens de uma única vez para os interessados, porém dependendo da frequência em que essa mensagem é disseminada pode causar congestionamento na rede, causando uma queda de desempenho, e enviando mensagens para máquinas que não estejam interessados nesses dados. Para resolver esse problema existe um modo de envio conhecido como Multicast que será abordado no próximo artigo.

## Referência
* [Link do projeto completo](https://github.com/NakedSolidSnake/Raspberry_IPC_Socket_UDP_Broadcast)
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)

