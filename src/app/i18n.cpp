#include "i18n.h"

namespace app::i18n {

const std::array<Translation, static_cast<std::size_t>(Text::Count)> translations{{
    {"自动", "自動", "Auto", "自動", "자동", "Auto", "Auto", "Automatisch", "Automático", "Авто"},
    {"主题", "主題", "Theme", "テーマ", "테마", "Tema", "Thème", "Design", "Tema", "Тема"},
    {"跟随系统", "跟隨系統", "System", "システムに従う", "시스템 설정", "Sistema", "Système", "System", "Sistema", "Система"},
    {"亮色", "亮色", "Light", "ライト", "라이트", "Claro", "Clair", "Hell", "Claro", "Светлая"},
    {"暗色", "暗色", "Dark", "ダーク", "다크", "Oscuro", "Sombre", "Dunkel", "Escuro", "Тёмная"},
    {"语言", "語言", "Language", "言語", "언어", "Idioma", "Langue", "Sprache", "Idioma", "Язык"},
    {"简体中文", "簡體中文", "Simplified Chinese", "簡体字中国語", "중국어 간체", "Chino simplificado", "Chinois simplifié", "Chinesisch (vereinfacht)", "Chinês simplificado", "Китайский (упрощённый)"},
    {"英文", "英文", "English", "英語", "영어", "Inglés", "Anglais", "Englisch", "Inglês", "Английский"},
    {"清空", "清除", "Clear", "クリア", "지우기", "Limpiar", "Effacer", "Leeren", "Limpar", "Очистить"},
    {"粘贴或输入文本", "貼上或輸入文字", "Paste or type text", "テキストを貼り付けまたは入力", "텍스트 붙여넣기 또는 입력", "Pega o escribe texto", "Collez ou saisissez du texte", "Text einfügen oder eingeben", "Cole ou digite o texto", "Вставьте или введите текст"},
    {"等待输入…", "等待輸入…", "Waiting for input…", "入力待ち…", "입력 대기 중…", "Esperando entrada…", "En attente de saisie…", "Warte auf Eingabe…", "Aguardando entrada…", "Ожидание ввода…"},
    {"处理中…", "處理中…", "Processing…", "処理中…", "처리 중…", "Procesando…", "Traitement…", "Verarbeitung…", "Processando…", "Обработка…"},
    {"无效", "無效", "Invalid", "無効", "유효하지 않음", "No válido", "Invalide", "Ungültig", "Inválido", "Недопустимо"},
    {"已复制", "已複製", "Copied", "コピー済み", "복사됨", "Copiado", "Copié", "Kopiert", "Copiado", "Скопировано"},
    {"字节", "位元組", "bytes", "バイト", "바이트", "bytes", "octets", "Bytes", "bytes", "байт"},
    {"格式化", "格式化", "Format", "整形", "서식", "Formatear", "Formater", "Formatieren", "Formatar", "Форматировать"},
    {"压缩", "壓縮", "Minify", "圧縮", "압축", "Minificar", "Minifier", "Minimieren", "Minificar", "Сжать"},
    {"转义", "逸出", "Escape", "エスケープ", "이스케이프", "Escapar", "Échapper", "Escapen", "Escapar", "Экранировать"},
    {"反转义", "取消逸出", "Unescape", "エスケープ解除", "이스케이프 해제", "Desescapar", "Déséchapper", "Escape aufheben", "Desescapar", "Снять экранирование"},
    {"表格", "表格", "Table", "テーブル", "표", "Tabla", "Tableau", "Tabelle", "Tabela", "Таблица"},
    {"自动分隔", "自動分隔", "Auto delimiter", "区切り自動", "구분자 자동", "Separador auto", "Séparateur auto", "Trennzeichen auto", "Separador auto", "Авторазделитель"},
    {"逗号", "逗號", "Comma", "カンマ", "쉼표", "Coma", "Virgule", "Komma", "Vírgula", "Запятая"},
    {"分号", "分號", "Semicolon", "セミコロン", "세미콜론", "Punto y coma", "Point-virgule", "Semikolon", "Ponto e vírgula", "Точка с запятой"},
    {"竖线", "豎線", "Pipe", "パイプ", "파이프", "Barra vertical", "Barre verticale", "Pipe", "Barra vertical", "Вертикальная черта"},
    {"表头", "標題列", "Header", "ヘッダー", "헤더", "Encabezado", "En-tête", "Kopfzeile", "Cabeçalho", "Заголовок"},
    {"开", "開", "On", "オン", "켜기", "Sí", "Oui", "Ein", "Ligado", "Вкл"},
    {"关", "關", "Off", "オフ", "끄기", "No", "Non", "Aus", "Desligado", "Выкл"},
    {"解码", "解碼", "Decode", "デコード", "디코드", "Decodificar", "Décoder", "Dekodieren", "Decodificar", "Декодировать"},
    {"再解一层", "再解一層", "Decode +1", "もう1層デコード", "한 번 더 디코딩", "Decodificar +1", "Décoder +1", "Nochmals dekodieren", "Decodificar +1", "Декодировать +1"},
    {"查看", "檢視", "Inspect", "表示", "보기", "Inspeccionar", "Inspecter", "Prüfen", "Inspecionar", "Просмотр"},
    {"Base64 编码", "Base64 編碼", "B64 Encode", "B64 エンコード", "B64 인코딩", "Codificar B64", "Encoder B64", "B64 kodieren", "Codificar B64", "Кодировать B64"},
    {"URL 编码", "URL 編碼", "URL Encode", "URL エンコード", "URL 인코딩", "Codificar URL", "Encoder URL", "URL kodieren", "Codificar URL", "Кодировать URL"},
    {"大写", "大寫", "Upper", "大文字", "대문자", "Mayúsculas", "Majuscules", "Groß", "Maiúsculas", "Верхний регистр"},
    {"小写", "小寫", "Lower", "小文字", "소문자", "Minúsculas", "Minuscules", "Klein", "Minúsculas", "Нижний регистр"},
    {"HTML 编码", "HTML 編碼", "HTML Encode", "HTML エンコード", "HTML 인코딩", "Codificar HTML", "Encoder HTML", "HTML kodieren", "Codificar HTML", "Кодировать HTML"},
    {"Hex 编码", "Hex 編碼", "Hex Encode", "Hex エンコード", "Hex 인코딩", "Codificar Hex", "Encoder Hex", "Hex kodieren", "Codificar Hex", "Кодировать Hex"},
    {"Unicode 转义", "Unicode 逸出", "Unicode Escape", "Unicode エスケープ", "Unicode 이스케이프", "Escapar Unicode", "Échapper Unicode", "Unicode escapen", "Escapar Unicode", "Экранировать Unicode"},
    {"时间", "時間", "Time", "時刻", "시간", "Tiempo", "Heure", "Zeit", "Hora", "Время"},
    {"摘要", "摘要", "Digest", "ダイジェスト", "다이제스트", "Resumen", "Empreinte", "Hash", "Resumo", "Хеш"},
    {"HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC"},
    {"HMAC 密钥（仅内存）", "HMAC 金鑰（僅記憶體）", "HMAC key (memory only)", "HMAC キー（メモリのみ）", "HMAC 키(메모리 전용)", "Clave HMAC (solo memoria)", "Clé HMAC (mémoire uniquement)", "HMAC-Schlüssel (nur Speicher)", "Chave HMAC (somente memória)", "Ключ HMAC (только в памяти)"},
    {"原始输入", "原始輸入", "Input", "入力", "입력", "Entrada", "Entrée", "Eingabe", "Entrada", "Ввод"},
    {"当前结果", "目前結果", "Result", "結果", "결과", "Resultado", "Résultat", "Ergebnis", "Resultado", "Результат"},
    {"密钥编码", "金鑰編碼", "Key encoding", "キーの形式", "키 인코딩", "Codificación", "Encodage de clé", "Schlüsselcodierung", "Codificação", "Кодировка ключа"},
    {"显示密钥", "顯示金鑰", "Show key", "キーを表示", "키 표시", "Mostrar clave", "Afficher la clé", "Schlüssel anzeigen", "Mostrar chave", "Показать ключ"},
    {"隐藏密钥", "隱藏金鑰", "Hide key", "キーを隠す", "키 숨기기", "Ocultar clave", "Masquer la clé", "Schlüssel ausblenden", "Ocultar chave", "Скрыть ключ"},
    {"弱算法，仅用于兼容性", "弱演算法，僅用於相容性", "Weak algorithm; compatibility use only", "弱いアルゴリズム（互換性のみ）", "약한 알고리즘, 호환성 전용", "Algoritmo débil; solo compatibilidad", "Algorithme faible ; compatibilité uniquement", "Schwacher Algorithmus; nur Kompatibilität", "Algoritmo fraco; apenas compatibilidade", "Слабый алгоритм; только для совместимости"},
    {"CRC32 仅用于错误检测，不适合安全用途", "CRC32 僅用於錯誤檢測，不適合安全用途", "CRC32 detects accidental changes; it is not cryptographic", "CRC32 は誤り検出用であり、暗号学的ではありません", "CRC32는 오류 검사용이며 암호학적이지 않습니다", "CRC32 detecta errores accidentales; no es criptográfico", "CRC32 détecte les erreurs accidentelles ; il n’est pas cryptographique", "CRC32 erkennt Übertragungsfehler und ist nicht kryptografisch", "CRC32 detecta erros acidentais; não é criptográfico", "CRC32 обнаруживает случайные ошибки и не является криптографическим"},
    {"预览前", "預覽前", "Previewing first", "先頭をプレビュー", "처음 항목 미리보기", "Vista previa de", "Aperçu des premiers", "Vorschau der ersten", "Prévia dos primeiros", "Показ первых"},
    {"行", "列", "rows", "行", "행", "filas", "lignes", "Zeilen", "linhas", "строк"},
    {"行 · 复制全部导出 TSV", "列 · 複製全部會匯出 TSV", "rows · Copy all exports TSV", "行・全コピーはTSVで出力", "행 · 전체 복사는 TSV로 내보내기", "filas · Copiar todo exporta TSV", "lignes · Tout copier exporte en TSV", "Zeilen · Alles kopieren exportiert TSV", "linhas · Copiar tudo exporta TSV", "строк · Копировать всё в TSV"},
    {"表格预览 · 复制全部导出 TSV", "表格預覽 · 複製全部會匯出 TSV", "Table preview · Copy all exports TSV", "テーブルプレビュー・全コピーはTSV", "표 미리보기 · 전체 복사는 TSV", "Vista de tabla · Copiar todo exporta TSV", "Aperçu du tableau · Tout copier exporte TSV", "Tabellenvorschau · Alles kopieren als TSV", "Prévia da tabela · Copiar tudo exporta TSV", "Предпросмотр · Копировать всё в TSV"},
    {"搜索结果", "搜尋結果", "Search result", "結果を検索", "결과 검색", "Buscar resultado", "Rechercher", "Ergebnis durchsuchen", "Pesquisar resultado", "Поиск в результате"},
    {"无效的正则表达式", "無效的正規表示式", "Invalid regular expression", "無効な正規表現", "잘못된 정규식", "Expresión regular no válida", "Expression régulière invalide", "Ungültiger regulärer Ausdruck", "Expressão regular inválida", "Недопустимое регулярное выражение"},
    {"复制全部", "全部複製", "Copy all", "すべてコピー", "전체 복사", "Copiar todo", "Tout copier", "Alles kopieren", "Copiar tudo", "Копировать всё"},
    {"复制选中", "複製所選", "Copy selected", "選択をコピー", "선택 복사", "Copiar selección", "Copier la sélection", "Auswahl kopieren", "Copiar seleção", "Копировать выбранное"},
    {"已选择", "已選取", "Selected", "選択済み", "선택됨", "Seleccionado", "Sélectionné", "Ausgewählt", "Selecionado", "Выбрано"},
    {"仅本地处理", "僅在本機處理", "Local only", "ローカルのみ", "로컬 전용", "Solo local", "Local uniquement", "Nur lokal", "Somente local", "Только локально"},
    {"关于", "關於", "About", "このアプリについて", "정보", "Acerca de", "À propos", "Info", "Sobre", "О программе"},
    {"版本", "版本", "Version", "バージョン", "버전", "Versión", "Version", "Version", "Versão", "Версия"},
    {"构建", "組建", "Build", "ビルド", "빌드", "Compilación", "Build", "Build", "Compilação", "Сборка"},
    {"本地优先的开发者文本格式化与编码工具。", "本機優先的開發者文字格式化與編碼工具。", "A local-first text formatting and encoding tool for developers.", "開発者向けのローカル優先テキスト整形・エンコードツールです。", "개발자를 위한 로컬 우선 텍스트 포맷 및 인코딩 도구입니다.", "Una herramienta local de formato y codificación de texto para desarrolladores.", "Un outil local de formatage et d’encodage de texte pour les développeurs.", "Ein lokal arbeitendes Textformatierungs- und Kodierungswerkzeug für Entwickler.", "Uma ferramenta local de formatação e codificação de texto para desenvolvedores.", "Локальный инструмент форматирования и кодирования текста для разработчиков."},
    {"所有内容仅在本机处理，不上传，也不保存历史记录。", "所有內容僅在本機處理，不上傳，也不儲存歷史記錄。", "All content stays on this device. Nothing is uploaded or saved to history.", "すべての内容はこの端末内で処理され、アップロードも履歴保存もされません。", "모든 콘텐츠는 이 기기에서만 처리되며 업로드하거나 기록에 저장하지 않습니다.", "Todo el contenido se procesa en este dispositivo; no se sube ni se guarda en el historial.", "Tout le contenu reste sur cet appareil, sans envoi ni historique.", "Alle Inhalte bleiben auf diesem Gerät und werden weder hochgeladen noch gespeichert.", "Todo o conteúdo permanece neste dispositivo, sem envio ou histórico.", "Все данные обрабатываются на этом устройстве, не загружаются и не сохраняются в историю."},
    {"特别感谢", "特別感謝", "Special thanks", "スペシャルサンクス", "특별히 감사드립니다", "Agradecimientos", "Remerciements", "Besonderer Dank", "Agradecimentos", "Особая благодарность"},
    {"感谢这些优秀的开源项目让 Zeus Tools 成为可能。", "感謝這些優秀的開源專案讓 Zeus Tools 成為可能。", "Zeus Tools is made possible by these open-source projects.", "これらのオープンソースプロジェクトに感謝します。", "Zeus Tools를 가능하게 한 오픈 소스 프로젝트에 감사드립니다.", "Zeus Tools es posible gracias a estos proyectos de código abierto.", "Zeus Tools existe grâce à ces projets open source.", "Diese Open-Source-Projekte machen Zeus Tools möglich.", "O Zeus Tools é possível graças a estes projetos de código aberto.", "Zeus Tools создан благодаря этим проектам с открытым исходным кодом."},
    {"项目主页", "專案首頁", "Project website", "プロジェクトサイト", "프로젝트 웹사이트", "Sitio del proyecto", "Site du projet", "Projektwebsite", "Site do projeto", "Сайт проекта"},
    {"MIT 许可证", "MIT 授權條款", "MIT License", "MIT ライセンス", "MIT 라이선스", "Licencia MIT", "Licence MIT", "MIT-Lizenz", "Licença MIT", "Лицензия MIT"},
    {"打开文件", "開啟檔案", "Open file", "ファイルを開く", "파일 열기", "Abrir archivo", "Ouvrir un fichier", "Datei öffnen", "Abrir arquivo", "Открыть файл"},
    {"导出结果", "匯出結果", "Export result", "結果を書き出す", "결과 내보내기", "Exportar resultado", "Exporter le résultat", "Ergebnis exportieren", "Exportar resultado", "Экспортировать результат"},
    {"保存二进制", "儲存二進位資料", "Save binary", "バイナリを保存", "바이너리 저장", "Guardar binario", "Enregistrer le binaire", "Binärdaten speichern", "Salvar binário", "Сохранить двоичные данные"},
    {"关闭", "關閉", "Close", "閉じる", "닫기", "Cerrar", "Fermer", "Schließen", "Fechar", "Закрыть"},
    {"输入超过 10 MiB", "輸入超過 10 MiB", "Input exceeds 10 MiB", "入力が 10 MiB を超えています", "입력이 10 MiB를 초과했습니다", "La entrada supera 10 MiB", "L’entrée dépasse 10 Mio", "Eingabe überschreitet 10 MiB", "A entrada excede 10 MiB", "Ввод превышает 10 МиБ"},
    {"大输入分页编辑", "大型輸入分頁編輯", "Paged large-input editor", "大きな入力のページ編集", "대용량 입력 페이지 편집", "Editor paginado de entrada grande", "Éditeur paginé pour grande entrée", "Seiteneditor für große Eingaben", "Editor paginado de entrada grande", "Постраничный редактор большого ввода"},
    {"仍然处理", "仍然處理", "Process anyway", "そのまま処理", "계속 처리", "Procesar de todos modos", "Traiter quand même", "Trotzdem verarbeiten", "Processar mesmo assim", "Всё равно обработать"},
    {"上一页", "上一頁", "Previous page", "前のページ", "이전 페이지", "Página anterior", "Page précédente", "Vorherige Seite", "Página anterior", "Предыдущая страница"},
    {"下一页", "下一頁", "Next page", "次のページ", "다음 페이지", "Página siguiente", "Page suivante", "Nächste Seite", "Próxima página", "Следующая страница"},
    {"复制完整输入", "複製完整輸入", "Copy full input", "入力全体をコピー", "전체 입력 복사", "Copiar entrada completa", "Copier toute l’entrée", "Gesamte Eingabe kopieren", "Copiar entrada completa", "Копировать весь ввод"},
}};

const char* get(Text text, zeus::LocalePreference locale) {
    const auto index = static_cast<std::size_t>(text);
    if (index >= translations.size()) return "";
    const auto& value = translations[index];
    switch (locale) {
    case zeus::LocalePreference::Chinese: return value.zh_hans;
    case zeus::LocalePreference::TraditionalChinese: return value.zh_hant;
    case zeus::LocalePreference::Japanese: return value.ja;
    case zeus::LocalePreference::Korean: return value.ko;
    case zeus::LocalePreference::Spanish: return value.es;
    case zeus::LocalePreference::French: return value.fr;
    case zeus::LocalePreference::German: return value.de;
    case zeus::LocalePreference::Portuguese: return value.pt;
    case zeus::LocalePreference::Russian: return value.ru;
    case zeus::LocalePreference::English:
    case zeus::LocalePreference::System: return value.en;
    }
    return value.en;
}


} // namespace app::i18n
