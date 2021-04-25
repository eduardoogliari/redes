Acadêmicos:
----------------
Eduardo Ogliari (egliari@gmail.com)
Guilherme Ferreira Sabino Da Silva (gfersasil@gmail.com)


Como compilar:
----------------
Execute um dos comandos:

> make debug
> make release


Funcionamento:
----------------
Execute o programa 'main' e passe como argumento de linha de comandoo ID do roteador a ser instanciado
(e.g. "./main 1" irá instanciar o roteador com ID 1)


Ao iniciar, o programa apresentará um menu com as seguintes opções:

#1. Enviar mensagem
    > Permite enviar uma mensagem de dados para um dos vizinhos
        (Nas Etapas 1 e 2 é enviado diretamente, porém na Etapa 3 o roteamento determina automaticamente o destino)

#2. Visualizar log
    > Permite visualizar o log do roteador, que registra alguns eventos como envio e recebimento de
        mensagens de dados e vetores distancia.

#3. Historico de mensagens
    > Permite visualizar as ultimas mensagens que foram destinadas a este roteador (dados e vetores)

#4. Visualizar tabela de vetores
    > Permite visualizar os valores atuais de cada um dos vetores distancia armazenados no roteador

#5. Sair
    > Encerra a execução do programa



Outras observações:
------------------
> O intervalo de envio do vetor distancia está definido pela macro "INTERVALO_ENVIO_DIST" presente no arquivo "main.c"
> O intervalo de tolerancia para recebimento de vetores, antes que o roteador considere um vetor como expirado é
    definido pela macro "TOLERANCIA_ATRASO_DIST", presente no arquivo "main.c"
