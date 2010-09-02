/*    $Id: russian.h,v 1.11 2005/08/16 00:28:08 rad2k Exp $    */

/*
 * YSM Language Specific header file
 *
 *    RUSSIAN LANGUAGE
 *
 * by rad2k
 * translated by yurasik (yurasik@isir.minsk.by)
 * translation completed by Evgueni V. Gavrilov <aquatique@rusunix.org>
 *
 */


/* YSM STARTING MESSAGES */

#define MSG_STARTING_ONE    ".." "����," " ������� �� ������� �� ��������..."
#define MSG_STARTING_TWO    "[ ������������� YSM ][ " "UIN" " A USAR :"
#define MSG_FOUND_SECPASS    "������ ��������� ������. ������ �� ���������� ;)."
#define MSG_FOUND_SECPASS2    "� ��� ���... [��� ��������]: "
#define MSG_READ_SLAVES        "YSM ���������� ����� � ���� ������:" " ["
#define MSG_ASK_KEYMAP1        "���������� �� ���������: "
#define MSG_ASK_KEYMAP2        "������� ������������� ����������? [Y/N]:"
#define MSG_NETWORK_INFO    "[���������� � ����: ��������� TCP ����:"
#define MSG_NETWORK_INFO2    "��, �������� ����� ���������� � �������"
#define MSG_NETWORK_INFO3    "����������� � BOS 2-�� �������..."
#define MSG_LOGIN_OK        "���� ��������!" " << ������� ������ � ���� ICQ! >>"
#define MSG_ASK_DOWNLOAD1    "�� ������� ��������� ������ ����������� ���������."
#define MSG_ASK_DOWNLOAD2    "������ ��� �������? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES    "downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1    "[������ #"
#define MSG_STATUS_CHANGE2    " �� �����" 
#define MSG_STATUS_CHANGE3    "������� ���������� ��:"

#define MSG_STATUS_HIDE1    "[������ #"
#define MSG_STATUS_HIDE2    ", �� �����" 
#define MSG_STATUS_HIDE3    "���������� �:"


#define MSG_MESSAGE_SENT1    "YSM ���������� ��������� ������ �� �����" 
#define MSG_MESSAGE_SENT2    "YSM ���������� ���������. ����������" 

#define MSG_MESSAGE_SENT3       "������������� ��������� ������� ������" 
#define MSG_MESSAGE_SENT4       "������������� ��������� �������" 

#define MSG_INCOMING_MESSAGE    "[�������� ���������]"
#define MSG_INCOMING_URL    "[�������� URL]"
#define MSG_INCOMING_AUTHR    "�������� ������ ����������� ��"

#define MSG_INCOMING_PAGER    "[��������� � ICQ-��������]"

#define MSG_SLAVES_LIST        "����� ������� �������� ������ ���������:"
#define MSG_SLAVES_ONLINE    "��� ���, �������! �������� ��������� � �������:"
#define MSG_SLAVES_BIRTHDAY    "BIRTHDAY" 

#define MSG_WARN_ADDED        "��� �� ��������! ��� ������ ��� �������� � ������ ���������!"
#define MSG_WARN_AUTHOK        "�������! ��� ������������. � ����������� � ���������!"
#define MSG_WARN_AUTHDENY    "�� �� ������?! ���� ��������� ������� ������� ��� � �����������!"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED    "���-���. ���� ����� �� ����� � ��� ���������..\n"
#define MSG_AFK_MODE_ON        "�� " "������ � ���" " ������ (������ �� ���������� [AFK]).\n�������� ��������� ����� �����������������, � � �������������\n��������� ������������.\n�������� 'afk' ����� ����� ����� �� ��� ������. ��������� ���� ;)"
#define MSG_AFK_MODE_OFF1    "��, ��� ������! � ��� �� �� ��������� �����"
#define MSG_AFK_MODE_OFF2    "�����!\n"
#define MSG_AFK_MODE_OFF3    "����� ��������� ���� �������� � ����� ysm\n� ����"
#define MSG_AFK_MODE_OFF4    "���� ��� �� ���� �����.\n�������� 'readafk', ����� ��������� ��� ������� ����������� ���������!.\n������� ��" " ��� " "������...\n"

#define MSG_AFK_READ_MSG    "[[ ������ ����������� ��� ��������� ]]" 

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH    "<<���!>>" " ������ �� ���������."
#define MSG_ERR_DISCONNECTED    "������ ������ ����������. ��������� ������."
#define MSG_ERR_INVPASSWD    "����� �������: ������������ ������." 
#define MSG_ERR_INVUIN        "����� �������: ������������ UIN." 
#define MSG_ERR_TOOMC        "����� �������: ������� ����� �������� � ������ IP." 
#define MSG_ERR_RATE        "����� �������: ���������� ������� ����������� ���������. ���������� �����." 
#define MSG_ERR_SAMEUIN        "!PARANOID!" " ���-�� ������ � ICQ � ����� UIN." 
#define MSG_ERR_CONNECT        "�� ������� ���������� ����������!"
#define MSG_ERR_PROXY        "������-������ ������ ������!::"
#define MSG_ERR_PROXY2        "������-������ ������� �����������.\n"
#define MSG_ERR_PROXY3        "Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING    "�� ����� ����������� ������ ��� ������������ �����. �������."

#define MSG_ERR_FEATUISABLED    "������� ���������. ������������� YSM � ���������� ������� (threads)."

#define MSG_CONN_PROXYOK    "����������� � ������-��������!"
#define MSG_CONN_SERVEROK    "����������� TCP-���������� � ICQ-��������"

#define MSG_AOL_WARNING        "�������������� AOL! �� ������� ������ ��������� ���������!"

#define MSG_REQ_SENT1        "YSM ������ ������"
#define MSG_REQ_SENT2        "������ � ������"

#define MSG_AUTH_SENT1        "YSM ����������� ����������"
#define MSG_AUTH_SENT2        "������ � ������"

#define MSG_NEW_OFFLINE        "���� ������� ���������, ���� �� ����" " ���������"

#define MSG_BUDDY_GROUPCREATED    "YSM ������ �������."
#define MSG_BUDDY_SAVING1    "��������� ["
#define MSG_BUDDY_SAVING2    "]" " ["
#define MSG_BUDDY_SAVING3    "]"    

#define MSG_BUDDY_SAVINGOK    " [OK] SLAVE SAVED"
#define MSG_BUDDY_SAVINGERROR    " [ERROR]"
#define MSG_BUDDY_SAVINGERROR2    "�������� ����������� ����������� �ޣ���� ������?."


#define MSG_BUDDY_SAVINGAUTH    " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2    "��������� ����������� ������������."
#define MSG_BUDDY_SAVINGAUTH3    " [����������� ������� 'req']."

#define MSG_BUDDY_BLOCKWARN    "�������� ���������� � ������� ������."
#define MSG_BUDDY_BLOCKWARN2    "���� � ���������� ����� ���ͣ������ �� ���� ������."

#define MSG_REG_SUCCESFULL    "�������� �����������. ��� ����� UIN "
#define MSG_REG_FAILED        "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." 

#define MSG_INFO_UPDATED    "���������� ���������!"

#define MSG_SEARCH_FAILED    "����� �� ������. �� ������� ��� ����������� ���������� UIN/Mail?"

#define MSG_DIRECT_CONNECTING    "��, ���������... ����������� � "
#define MSG_DIRECT_ESTABLISHED    "���������� �����������."
#define MSG_DIRECT_ERR1    "�� ���� ���������� ������ ���������� ������� ����� ������-������."
#define MSG_DIRECT_ERR2    "�� ���� ���������� ������ ����������.\n���������� IP-����� ��� ���� ���̣���� �������."
#define MSG_DIRECT_ERR3 "�� ���� ���������� ������ ����������.\n���̣���� ������� ��� ��������."
