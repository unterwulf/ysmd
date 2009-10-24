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

#define MSG_STARTING_ONE    ".." "Опыт," " который Вы никогда не забудете..."
#define MSG_STARTING_TWO    "[ ИНИЦИАЛИЗАЦИЯ YSM ][ " "UIN" " A USAR :"
#define MSG_FOUND_SECPASS    "Найден секретный пароль. Балдею от параноиков ;)."
#define MSG_FOUND_SECPASS2    "И еще раз... [для проверки]: "
#define MSG_READ_SLAVES        "YSM количество жертв в базе данных:" " ["
#define MSG_ASK_KEYMAP1        "Сокращения по умолчанию: "
#define MSG_ASK_KEYMAP2        "Желаете перенастроить сокращения? [Y/N]:"
#define MSG_NETWORK_INFO    "[Информация о сети: локальный TCP порт:"
#define MSG_NETWORK_INFO2    "Ок, получена новая информация с сервера"
#define MSG_NETWORK_INFO3    "Подключение к BOS 2-му серверу..."
#define MSG_LOGIN_OK        "Вход успешный!" " << Милости просим в сеть ICQ! >>"
#define MSG_ASK_DOWNLOAD1    "На сервере обнаружен список сохраненных контактов."
#define MSG_ASK_DOWNLOAD2    "Хотите его скачать? [Y/N]: "
#define MSG_DOWNLOADED_SLAVES    "downloaded SLAVES belong to your kingdom."

/* YSM SLAVES INTERACTION MESSAGES */

#define MSG_STATUS_CHANGE1    "[жертва #"
#define MSG_STATUS_CHANGE2    " по имени" 
#define MSG_STATUS_CHANGE3    "сменила настроение на:"

#define MSG_STATUS_HIDE1    "[жертва #"
#define MSG_STATUS_HIDE2    ", по имени" 
#define MSG_STATUS_HIDE3    "спряталась в:"


#define MSG_MESSAGE_SENT1    "YSM Отправлено сообщение жертве по имени" 
#define MSG_MESSAGE_SENT2    "YSM Отправлено сообщение. Получатель" 

#define MSG_MESSAGE_SENT3       "зашифрованное сообщение послано жертве" 
#define MSG_MESSAGE_SENT4       "зашифрованное сообщение послано" 

#define MSG_INCOMING_MESSAGE    "[Входящее сообщение]"
#define MSG_INCOMING_URL    "[Входящий URL]"
#define MSG_INCOMING_AUTHR    "Входящий запрос авторизации от"

#define MSG_INCOMING_PAGER    "[сообщение с ICQ-пэйджера]"

#define MSG_SLAVES_LIST        "Далее следует перечень Вашего хозяйства:"
#define MSG_SLAVES_ONLINE    "Вот они, родимые! Бездумно смотрящие в монитор:"
#define MSG_SLAVES_BIRTHDAY    "BIRTHDAY" 

#define MSG_WARN_ADDED        "Что за произвол! Вас только что добавили в список контактов!"
#define MSG_WARN_AUTHOK        "Отлично! Вас авторизовали. С пополнением в хозяйстве!"
#define MSG_WARN_AUTHDENY    "Ну вы видели?! Этот нехороший человек отказал Вам в авторизации!"

/* YSM AFK RELATED MESSAGES */

#define MSG_IDLE_TIME_REACHED    "Баю-бай. Даже когда Вы спите я ВСЕ записываю..\n"
#define MSG_AFK_MODE_ON        "Вы " "сейчас в ДОК" " режиме (Далеко От Клавиатуры [AFK]).\nВходящие сообщения будут законспектированы, а с нетерпеливыми\nпоговорит автоответчик.\nНаберите 'afk' снова чтобы выйти из ДОК режима. Нескучных снов ;)"
#define MSG_AFK_MODE_OFF1    "Ой, кто пришел! И где же Вы пропадали целых"
#define MSG_AFK_MODE_OFF2    "минут!\n"
#define MSG_AFK_MODE_OFF3    "новых сообщений были записаны в папке ysm\nв файл"
#define MSG_AFK_MODE_OFF4    "пока Вас не было рядом.\nНаберите 'readafk', чтобы прочитать или удалить сохраненные сообщения!.\nВыходим из" " ДОК " "режима...\n"

#define MSG_AFK_READ_MSG    "[[ Читаем сохраненные ДОК сообщения ]]" 

/* ERROR MESSAGES */

#define MSG_ERR_PASSMATCH    "<<Нет!>>" " Пароли НЕ совпадают."
#define MSG_ERR_DISCONNECTED    "Сервер отверг соединение. Проверьте пароль."
#define MSG_ERR_INVPASSWD    "Ответ сервера: неправильный пароль." 
#define MSG_ERR_INVUIN        "Ответ сервера: неправильный UIN." 
#define MSG_ERR_TOOMC        "Ответ сервера: слишком много клиентов с одного IP." 
#define MSG_ERR_RATE        "Ответ сервера: количество попыток соединиться исчерпано. Попробуйте позже." 
#define MSG_ERR_SAMEUIN        "!PARANOID!" " Кто-то другой в ICQ в Вашим UIN." 
#define MSG_ERR_CONNECT        "Не удалось установить соединение!"
#define MSG_ERR_PROXY        "Прокси-сервер вернул ошибку!::"
#define MSG_ERR_PROXY2        "Прокси-сервер требует авторизации.\n"
#define MSG_ERR_PROXY3        "Method Failed. Unknown Reason.\n"

#define MSG_ERR_REGISTERING    "Во время регистрации сервер дал неправильный ответ. Выходим."

#define MSG_ERR_FEATUISABLED    "Функция отключена. Скомпилируйте YSM с поддержкой потоков (threads)."

#define MSG_CONN_PROXYOK    "Соединились с прокси-сервером!"
#define MSG_CONN_SERVEROK    "Установлено TCP-соединение с ICQ-сервером"

#define MSG_AOL_WARNING        "Предупреждение AOL! Вы слишком быстро посылаете сообщения!"

#define MSG_REQ_SENT1        "YSM запрос послан"
#define MSG_REQ_SENT2        "жертве с именем"

#define MSG_AUTH_SENT1        "YSM авторизация отправлена"
#define MSG_AUTH_SENT2        "жертве с именем"

#define MSG_NEW_OFFLINE        "Было принято сообщение, пока Вы были" " отключены"

#define MSG_BUDDY_GROUPCREATED    "YSM группа создана."
#define MSG_BUDDY_SAVING1    "сохраняем ["
#define MSG_BUDDY_SAVING2    "]" " ["
#define MSG_BUDDY_SAVING3    "]"    

#define MSG_BUDDY_SAVINGOK    " [OK] SLAVE SAVED"
#define MSG_BUDDY_SAVINGERROR    " [ERROR]"
#define MSG_BUDDY_SAVINGERROR2    "Возможно добавлялась отключенная учётная запись?."


#define MSG_BUDDY_SAVINGAUTH    " [AUTH]"
#define MSG_BUDDY_SAVINGAUTH2    "Требуется авторизация пользователя."
#define MSG_BUDDY_SAVINGAUTH3    " [используйте команду 'req']."

#define MSG_BUDDY_BLOCKWARN    "Принимаю информацию о статусе жертвы."
#define MSG_BUDDY_BLOCKWARN2    "Ввод с клавиатуры может подмёрзнуть на пару секунд."

#define MSG_REG_SUCCESFULL    "Успешная регистрация. Ваш новый UIN "
#define MSG_REG_FAILED        "UIN Registration failed. This may be due to too many registration requests from the same IP address. Please try again later." 

#define MSG_INFO_UPDATED    "Информация обновлена!"

#define MSG_SEARCH_FAILED    "Поиск не удался. Вы уверены что используете правильный UIN/Mail?"

#define MSG_DIRECT_CONNECTING    "Ну, держитесь... Соединяемся с "
#define MSG_DIRECT_ESTABLISHED    "Соединение установлено."
#define MSG_DIRECT_ERR1    "Не могу установить прямое соединение работая через прокси-сервер."
#define MSG_DIRECT_ERR2    "Не могу установить прямое соединение.\nНеизвестен IP-адрес или порт удалённой стороны."
#define MSG_DIRECT_ERR3 "Не могу установить прямое соединение.\nУдалённая сторона его отвергла."
