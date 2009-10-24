/*	$Id: portuguesebr.h,v 1.10 2005/08/16 00:28:08 rad2k Exp $	*/

/*
 * YSM Language Specific header file
 *
 *	BRAZILIAN PORTUGUESE LANGUAGE
 *
 * by rad2k
 * translated by brc (brc@thz.net)
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE	"..uma" WHITE " Experiência" NORMAL " que você nunca mais vai esquecer.."
#define MSG_STARTING_TWO	"[ INICIALIZANDO YSM ][ " BLUE "UIN" NORMAL " A USAR :"
#define MSG_FOUND_SECPASS	"Senha segura encontrada. Eu adoro usuários paranóicos ;)."
#define MSG_FOUND_SECPASS2	"Digite dinovo [verificação]: "
#define MSG_READ_SLAVES		"YSM número de escravos no bd" BRIGHT_BLUE " ["
#define MSG_ASK_KEYMAP1		"Atalhos padrões são: "
#define MSG_ASK_KEYMAP2		"Você deseja reconfigurar seu mapa de teclado? [Y/N]:"
#define MSG_NETWORK_INFO	"[Informações da rede: porta local TCP:"
#define MSG_NETWORK_INFO2	"Ok nova informação srv. recebida."
#define MSG_NETWORK_INFO3	"Conectando ao BOS, segundo servidor.."
#define MSG_LOGIN_OK		BLUE "Login OK!" NORMAL " << Bem vindo a rede ICQ >>"
#define MSG_ASK_DOWNLOAD1	"Contatos guardados nos servidores do icq encontrados."
#define MSG_ASK_DOWNLOAD2	"Você deseja baixá-los? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES	"downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1	"[Um Slave #"
#define MSG_STATUS_CHANGE2	", chamado" BLUE
#define MSG_STATUS_CHANGE3	NORMAL "mudou seu status para:"

#define MSG_STATUS_HIDE1	"[A Slave #"
#define MSG_STATUS_HIDE2	", called" BLUE
#define MSG_STATUS_HIDE3	NORMAL "hides from you in:"


#define MSG_MESSAGE_SENT1	MAGENTA "YSM mensagem enviada ao Slave chamado" WHITE
#define MSG_MESSAGE_SENT2	MAGENTA "YSM mensagem enviada a" WHITE

#define MSG_MESSAGE_SENT3	MAGENTA	"Encrypted Msg Sent to Slave called" WHITE
#define MSG_MESSAGE_SENT4	MAGENTA	"Encrypted Msg Sent to" WHITE

#define MSG_INCOMING_MESSAGE	"[Nova mensagem]"
#define MSG_INCOMING_URL	"[Nova URL]"
#define MSG_INCOMING_AUTHR	"Pedido de autorização feito por"

#define MSG_INCOMING_PAGER	"[Pager Message]"

#define MSG_SLAVES_LIST		"Te pertencem os seguintes:"
#define MSG_SLAVES_ONLINE	"Esses são os SLAVES ONLINE:"

#define MSG_SLAVES_BIRTHDAY	WHITE "BIRTHDAY" NORMAL

#define MSG_WARN_ADDED		"Atenção! Um usuário do ICQ adicionou você para sua lista!"
#define MSG_WARN_AUTHOK		"Ok! Você foi autorizado a fazer dele/dela seu ICQ Slave"
#define MSG_WARN_AUTHDENY	"Puta Merda! O babaca -REJEITOU- seu pedido de autorização"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED	"Tempo de IDLE atingido. Entrando no modo afk..\n"
#define MSG_AFK_MODE_ON		"Você está agora no" BRIGHT_BLUE "modo AFK" NORMAL " (Away From Keyboard).\nNovas mensagens serão logadas e uma resposta automática será enviada\npara qualquer um que te mande uma mensagem.\nVocê pode digitar 'afk' para sair do modo AFK. Aproveite seu café ;)"

#define MSG_AFK_MODE_OFF1	"Bem vindo de volta! Você estava away"
#define MSG_AFK_MODE_OFF2	"minutos!.\n"
#define MSG_AFK_MODE_OFF3	"novas mensagens foram armazenadas no seu ysm\ndiretório no arquivo"
#define MSG_AFK_MODE_OFF4	"Enquanto você estava away.\nUse 'readafk' para ler ou apagar as mensagens armazenadas!.\nSaindo do" BRIGHT_BLUE " modo AFK " NORMAL "..\n"

#define MSG_AFK_READ_MSG	GREEN "[[ Lendo mensagens AFK armazenadas ]]" NORMAL

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH	CYAN "<<NÃO!>>" NORMAL " Senhas NÃO são iguais."
#define MSG_ERR_DISCONNECTED	"Desconectado do servidor. Verifique sua senha."
#define MSG_ERR_INVPASSWD	RED "Resposta do servidor: Senha incorreta." NORMAL
#define MSG_ERR_INVUIN		RED "Resposta do servidor: UIN inválido." NORMAL
#define MSG_ERR_TOOMC		RED "Resposta do servidor: Clientes demais acessando desse mesmo ip." NORMAL
#define MSG_ERR_RATE		RED "Resposta do servidor: Limite de rate excedido. Tente dinovo mais tarde." NORMAL
#define MSG_ERR_SAMEUIN		CYAN "!PARANOID!" NORMAL " Someone else logged in using this ICQ UIN." 
#define MSG_ERR_CONNECT		"Conexão falhou!"
#define MSG_ERR_PROXY		"Proxy retornou um ERRO!::"
#define MSG_ERR_PROXY2		"Proxy requires Authorization.\n"
#define MSG_ERR_PROXY3		"Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING	"Resposta do servidor inválida durante registro. Saindo."

#define MSG_ERR_FEATUREDISABLED	"Recurso desabilitado. Recompile YSM com suporte para Threads."

#define MSG_CONN_PROXYOK	"Conectado ao PROXY!"
#define MSG_CONN_SERVEROK	"Conexão TCP estabelecida com o SERVIDOR DE ICQ"

#define MSG_AOL_WARNING		"Alerta AOL! Pare de mandar msgs tão rápido!"

#define MSG_REQ_SENT1		"Pedido YSM mandado para"
#define MSG_REQ_SENT2		"Slave chamado"

#define MSG_AUTH_SENT1		"Autorização YSM mandada para"
#define MSG_AUTH_SENT2		"Slave chamado"

#define MSG_NEW_OFFLINE		"Mensagem recebida quando você estava" BRIGHT_BLUE " offline"

#define MSG_BUDDY_GROUPCREATED	"Grupo YSM Criado."
#define MSG_BUDDY_SAVING1	BRIGHT_BLUE "Salvando ["
#define MSG_BUDDY_SAVING2	"]" WHITE " ["
#define MSG_BUDDY_SAVING3	"]" NORMAL	

#define MSG_BUDDY_SAVINGOK	GREEN " [OK] SLAVE SALVO"
#define MSG_BUDDY_SAVINGERROR	RED " [ERRO]"
#define MSG_BUDDY_SAVINGERROR2	"Talvez adicionando uma conta desabilitada?."


#define MSG_BUDDY_SAVINGAUTH	RED " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2	"Esse usuário requer Autorização.."
#define MSG_BUDDY_SAVINGAUTH3	" [use o comando 'req']."

#define MSG_BUDDY_BLOCKWARN	"Recebendo informação sobre status do Slave."
#define MSG_BUDDY_BLOCKWARN2	"Seu input pode ser bloqueado por alguns segundos."

#define MSG_REG_SUCCESFULL	"Registro efetuado com sucesso. Seu novo UIN é "
#define MSG_REG_FAILED		RED "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." NORMAL

#define MSG_INFO_UPDATED	"Informação Atualizada!"

#define MSG_SEARCH_FAILED	"Procura Falhou. UIN/Mail validos?"

#define MSG_DIRECT_CONNECTING	"Aguarde. Conectando em "
#define MSG_DIRECT_ESTABLISHED	"Conexão estabelecida."
#define MSG_DIRECT_ERR1	"Não foi possível estabelecer uma conexão direta atraves de um proxy."
#define MSG_DIRECT_ERR2	"Não é possível estabelecer uma conexão direta.\nOu o IP remoto ou a Porta de entrada  são desconhecidos."
#define MSG_DIRECT_ERR3 "Não é possível estabelecer uma conexão direta.\nConexão recusada"
