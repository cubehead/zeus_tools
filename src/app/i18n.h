#pragma once

#include "zeus/locale_preference.h"

#include <array>
#include <cstddef>

namespace app::i18n {

enum class Text {
    Auto, Theme, System, Light, Dark, Language, Chinese, English, Clear,
    PasteOrTypeJson, WaitingForInput, Processing, Invalid, Copied, Bytes,
    Format, Minify, Escape, Unescape, Table, DelimiterAuto, DelimiterComma,
    DelimiterSemicolon, DelimiterPipe, FirstRowHeader, On, Off, Decode,
    Inspect, Base64Encode, UrlEncode, Upper, Lower, Digest, Hmac, HmacKey,
    WeakAlgorithm, PreviewingFirst, Rows, RowsCopyTsv, TablePreviewCopyTsv,
    SearchResult, InvalidRegex, CopyAll, CopySelected, Selected, LocalOnly, Count,
};

struct Translation {
    const char* zh_hans;
    const char* zh_hant;
    const char* en;
    const char* ja;
    const char* ko;
    const char* es;
    const char* fr;
    const char* de;
    const char* pt;
    const char* ru;
};

inline constexpr std::array<Translation, static_cast<std::size_t>(Text::Count)> translations{{
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
    {"查看", "檢視", "Inspect", "表示", "보기", "Inspeccionar", "Inspecter", "Prüfen", "Inspecionar", "Просмотр"},
    {"Base64 编码", "Base64 編碼", "B64 Encode", "B64 エンコード", "B64 인코딩", "Codificar B64", "Encoder B64", "B64 kodieren", "Codificar B64", "Кодировать B64"},
    {"URL 编码", "URL 編碼", "URL Encode", "URL エンコード", "URL 인코딩", "Codificar URL", "Encoder URL", "URL kodieren", "Codificar URL", "Кодировать URL"},
    {"大写", "大寫", "Upper", "大文字", "대문자", "Mayúsculas", "Majuscules", "Groß", "Maiúsculas", "Верхний регистр"},
    {"小写", "小寫", "Lower", "小文字", "소문자", "Minúsculas", "Minuscules", "Klein", "Minúsculas", "Нижний регистр"},
    {"摘要", "摘要", "Digest", "ダイジェスト", "다이제스트", "Resumen", "Empreinte", "Hash", "Resumo", "Хеш"},
    {"HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC", "HMAC"},
    {"HMAC 密钥（仅内存）", "HMAC 金鑰（僅記憶體）", "HMAC key (memory only)", "HMAC キー（メモリのみ）", "HMAC 키(메모리 전용)", "Clave HMAC (solo memoria)", "Clé HMAC (mémoire uniquement)", "HMAC-Schlüssel (nur Speicher)", "Chave HMAC (somente memória)", "Ключ HMAC (только в памяти)"},
    {"弱算法，仅用于兼容性", "弱演算法，僅用於相容性", "Weak algorithm; compatibility use only", "弱いアルゴリズム（互換性のみ）", "약한 알고리즘, 호환성 전용", "Algoritmo débil; solo compatibilidad", "Algorithme faible ; compatibilité uniquement", "Schwacher Algorithmus; nur Kompatibilität", "Algoritmo fraco; apenas compatibilidade", "Слабый алгоритм; только для совместимости"},
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
}};

inline const char* get(Text text, zeus::LocalePreference locale) {
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
