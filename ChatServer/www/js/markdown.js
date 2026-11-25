/**
 * 轻量 Markdown 渲染 + 代码高亮（无第三方依赖，适配流式增量渲染）
 * 用法：Markdown.render(text) -> HTML 字符串
 */
(function (global) {
  'use strict';

  /* ==================== 转义 ==================== */

  var ESCAPE_MAP = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' };

  function escapeHtml(input) {
    return String(input).replace(/[&<>"']/g, function (ch) {
      return ESCAPE_MAP[ch];
    });
  }

  /* ==================== 语法高亮 ==================== */

  var KEYWORDS = {
    js: 'await async break case catch class const continue debugger default delete do else export extends finally for from function if import in instanceof let new of return static super switch this throw try typeof var void while with yield true false null undefined interface type enum implements declare namespace abstract readonly public private protected',
    python: 'and as assert async await break class continue def del elif else except False finally for from global if import in is lambda None nonlocal not or pass raise return True try while with yield match case self',
    c: 'auto bool break case catch char class const constexpr continue default delete do double else enum explicit extern false float for friend goto if inline int long namespace new nullptr operator private protected public register return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while override final',
    go: 'break case chan const continue default defer else fallthrough for func go goto if import interface map package range return select struct switch type var true false nil string int int64 int32 float64 float32 byte rune bool error make len cap append delete panic recover',
    rust: 'as async await break const continue crate dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait true type unsafe use where while Some None Ok Err',
    sql: 'select from where and or not insert into values update set delete join left right inner outer on group by order having limit offset as distinct count sum avg min max create table primary key foreign references index drop alter add column null is in like between union all case when then else end asc desc',
    bash: 'if then else elif fi for while until do done case esac function in return exit echo export local readonly shift break continue sudo cd ls mkdir rm cp mv cat grep awk sed',
    css: 'important media supports keyframes import font-face from to root',
    json: 'true false null'
  };

  var LANG_ALIAS = {
    js: 'js', javascript: 'js', mjs: 'js', ts: 'js', typescript: 'js', jsx: 'js', tsx: 'js', node: 'js',
    py: 'python', python: 'python', python3: 'python',
    c: 'c', h: 'c', cpp: 'c', cc: 'c', cxx: 'c', hpp: 'c', 'c++': 'c', cs: 'c', csharp: 'c',
    java: 'c', kotlin: 'c', kt: 'c', swift: 'c', php: 'c', dart: 'c', scala: 'c', objc: 'c',
    go: 'go', golang: 'go',
    rust: 'rust', rs: 'rust',
    sql: 'sql', mysql: 'sql', sqlite: 'sql', postgresql: 'sql',
    sh: 'bash', bash: 'bash', shell: 'bash', zsh: 'bash', console: 'bash', terminal: 'bash',
    json: 'json', jsonc: 'json',
    css: 'css', scss: 'css', less: 'css', sass: 'css',
    html: 'markup', xml: 'markup', svg: 'markup', vue: 'markup', htm: 'markup'
  };

  var GRAMMAR = {
    js: { kw: KEYWORDS.js, line: '//', block: ['/*', '*/'], template: true },
    python: { kw: KEYWORDS.python, line: '#', block: null, triple: true },
    c: { kw: KEYWORDS.c, line: '//', block: ['/*', '*/'] },
    go: { kw: KEYWORDS.go, line: '//', block: ['/*', '*/'] },
    rust: { kw: KEYWORDS.rust, line: '//', block: ['/*', '*/'] },
    sql: { kw: KEYWORDS.sql, line: '--', block: ['/*', '*/'] },
    bash: { kw: KEYWORDS.bash, line: '#', block: null },
    css: { kw: KEYWORDS.css, line: null, block: ['/*', '*/'] },
    json: { kw: KEYWORDS.json, line: null, block: null }
  };

  function toKeywordSet(text) {
    var set = Object.create(null);
    String(text).split(/\s+/).forEach(function (word) {
      if (word) set[word] = true;
    });
    return set;
  }

  function span(cls, text) {
    return '<span class="tok-' + cls + '">' + escapeHtml(text) + '</span>';
  }

  function readString(code, start, quote, allowMultiline) {
    var i = start + 1;
    var n = code.length;
    while (i < n) {
      var ch = code[i];
      if (ch === '\\') { i += 2; continue; }
      if (ch === quote) { i += 1; break; }
      if (!allowMultiline && ch === '\n') break;
      i += 1;
    }
    return Math.min(i, n);
  }

  /* 通用类 C 语法高亮 */
  function highlightGeneric(code, lang) {
    var grammar = GRAMMAR[lang];
    if (!grammar) return escapeHtml(code);

    var keywords = toKeywordSet(grammar.kw);
    var out = [];
    var plain = '';
    var i = 0;
    var n = code.length;

    function flush() {
      if (plain) { out.push(escapeHtml(plain)); plain = ''; }
    }
    function push(cls, text) {
      flush();
      out.push(span(cls, text));
    }

    while (i < n) {
      var ch = code[i];

      // 行注释
      if (grammar.line && code.indexOf(grammar.line, i) === i) {
        var lineEnd = code.indexOf('\n', i);
        if (lineEnd === -1) lineEnd = n;
        push('com', code.slice(i, lineEnd));
        i = lineEnd;
        continue;
      }

      // 块注释
      if (grammar.block && code.indexOf(grammar.block[0], i) === i) {
        var blockEnd = code.indexOf(grammar.block[1], i + grammar.block[0].length);
        blockEnd = blockEnd === -1 ? n : blockEnd + grammar.block[1].length;
        push('com', code.slice(i, blockEnd));
        i = blockEnd;
        continue;
      }

      // 三引号字符串（Python 文档字符串）
      if (grammar.triple && (ch === '"' || ch === "'") && code.substr(i, 3) === ch + ch + ch) {
        var tripleEnd = code.indexOf(ch + ch + ch, i + 3);
        tripleEnd = tripleEnd === -1 ? n : tripleEnd + 3;
        push('str', code.slice(i, tripleEnd));
        i = tripleEnd;
        continue;
      }

      // 普通字符串
      if (ch === '"' || ch === "'" || (grammar.template && ch === '`')) {
        var strEnd = readString(code, i, ch, !!grammar.template && ch === '`');
        push('str', code.slice(i, strEnd));
        i = strEnd;
        continue;
      }

      // 数字
      if (ch >= '0' && ch <= '9') {
        var numMatch = /^(?:0[xX][0-9a-fA-F_]+|0[bB][01_]+|\d[\d_]*(?:\.[\d_]+)?(?:[eE][+-]?\d+)?)/.exec(code.slice(i));
        var numText = numMatch ? numMatch[0] : ch;
        push('num', numText);
        i += numText.length;
        continue;
      }

      // 标识符 / 关键字 / 函数名
      if (/[A-Za-z_$]/.test(ch)) {
        var j = i + 1;
        while (j < n && /[A-Za-z0-9_$]/.test(code[j])) j += 1;
        var word = code.slice(i, j);

        if (keywords[word]) {
          push('kw', word);
        } else {
          var k = j;
          while (k < n && (code[k] === ' ' || code[k] === '\t')) k += 1;
          if (code[k] === '(') push('fn', word);
          else plain += word;
        }
        i = j;
        continue;
      }

      plain += ch;
      i += 1;
    }

    flush();
    return out.join('');
  }

  /* HTML / XML 高亮 */
  function highlightMarkup(code) {
    var out = [];
    var last = 0;
    var re = /<!--[\s\S]*?(?:-->|$)|<!\[CDATA\[[\s\S]*?(?:\]\]>|$)|<\/?[A-Za-z][^>]*(?:>|$)/g;
    var match;

    while ((match = re.exec(code)) !== null) {
      out.push(escapeHtml(code.slice(last, match.index)));
      var token = match[0];

      if (token.indexOf('<!--') === 0 || token.indexOf('<![CDATA[') === 0) {
        out.push(span('com', token));
      } else {
        var parts = /^(<\/?)([A-Za-z][\w:.-]*)([\s\S]*?)(\/?>|$)/.exec(token);
        if (parts) {
          out.push(span('tag', parts[1] + parts[2]));
          out.push(highlightAttrs(parts[3]));
          out.push(span('tag', parts[4]));
        } else {
          out.push(escapeHtml(token));
        }
      }
      last = re.lastIndex;
    }

    out.push(escapeHtml(code.slice(last)));
    return out.join('');
  }

  function highlightAttrs(raw) {
    var out = [];
    var last = 0;
    var re = /([A-Za-z_:][\w:.-]*)(\s*=\s*)("[^"]*"|'[^']*'|[^\s>]+)?/g;
    var match;

    while ((match = re.exec(raw)) !== null) {
      out.push(escapeHtml(raw.slice(last, match.index)));
      out.push(span('attr', match[1]));
      if (match[2]) out.push(escapeHtml(match[2]));
      if (match[3]) out.push(span('str', match[3]));
      last = re.lastIndex;
    }

    out.push(escapeHtml(raw.slice(last)));
    return out.join('');
  }

  function normalizeLang(raw) {
    var key = String(raw || '').toLowerCase().trim();
    return key ? (LANG_ALIAS[key] || '') : '';
  }

  function sniffLang(code) {
    var text = code.trim();
    if (!text) return 'text';
    if (/^[[{]/.test(text) && /"[^"]*"\s*:/.test(text)) return 'json';
    if (/^<\s*[!?a-zA-Z]/.test(text)) return 'markup';
    if (/^\s*(def|class|import|from)\s+\w+/m.test(text)) return 'python';
    if (/(function|const |let |var |=>|console\.log)/.test(text)) return 'js';
    if (/(#include|std::|printf|int main)/.test(text)) return 'c';
    if (/^(SELECT|INSERT|UPDATE|DELETE|CREATE)\b/i.test(text)) return 'sql';
    return 'text';
  }

  function highlight(code, lang) {
    if (!code) return '';
    if (lang === 'markup') return highlightMarkup(code);
    return highlightGeneric(code, lang);
  }

  function renderCodeBlock(code, rawLang) {
    var lang = normalizeLang(rawLang) || sniffLang(code);
    var shown = lang === 'text' ? (rawLang || 'text') : lang;
    var body = lang === 'text' ? escapeHtml(code) : highlight(code, lang);

    return '<div class="code-block">' +
      '<div class="code-head">' +
        '<span class="code-lang">' + escapeHtml(shown) + '</span>' +
        '<button class="code-copy" type="button">复制</button>' +
      '</div>' +
      '<pre><code>' + body + '</code></pre>' +
    '</div>';
  }

  /* ==================== 行内解析 ==================== */

  function renderInline(text) {
    var codes = [];

    // 先抽出行内代码，避免后续规则破坏其内容
    var src = String(text).replace(/`([^`\n]+)`/g, function (m, code) {
      codes.push(code);
      return '\u0000' + (codes.length - 1) + '\u0000';
    });

    src = escapeHtml(src);

    // 链接（仅允许安全协议）
    src = src.replace(/\[([^\]]*)\]\(([^)\s]+)\)/g, function (m, label, href) {
      if (!/^(https?:\/\/|mailto:|\/|#)/i.test(href)) return label;
      return '<a href="' + href + '" target="_blank" rel="noopener noreferrer">' + label + '</a>';
    });

    // 粗体 / 斜体 / 删除线
    src = src.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    src = src.replace(/__([^_]+)__/g, '<strong>$1</strong>');
    src = src.replace(/(^|[^*])\*([^*\n]+)\*/g, '$1<em>$2</em>');
    src = src.replace(/(^|[^_\w])_([^_\n]+)_/g, '$1<em>$2</em>');
    src = src.replace(/~~([^~]+)~~/g, '<del>$1</del>');

    // 还原行内代码
    src = src.replace(/\u0000(\d+)\u0000/g, function (m, index) {
      return '<code class="inline-code">' + escapeHtml(codes[Number(index)]) + '</code>';
    });

    return src;
  }

  /* ==================== 块级解析 ==================== */

  var RE_FENCE = /^ {0,3}(`{3,}|~{3,})\s*([A-Za-z0-9#+._-]*)\s*$/;
  var RE_HEADING = /^ {0,3}(#{1,6})\s+(.*)$/;
  var RE_HR = /^ {0,3}(?:(?:\*\s*){3,}|(?:-\s*){3,}|(?:_\s*){3,})$/;
  var RE_QUOTE = /^ {0,3}>/;
  var RE_UL = /^ {0,3}[-*+]\s+/;
  var RE_OL = /^ {0,3}\d+[.)]\s+/;

  function isBlockStart(line) {
    return RE_FENCE.test(line) || RE_HEADING.test(line) || RE_HR.test(line) ||
           RE_QUOTE.test(line) || RE_UL.test(line) || RE_OL.test(line);
  }

  function render(text) {
    if (text === null || text === undefined) return '';

    var lines = String(text).replace(/\r\n?/g, '\n').split('\n');
    var html = '';
    var i = 0;

    while (i < lines.length) {
      var line = lines[i];

      // 围栏代码块
      var fence = RE_FENCE.exec(line);
      if (fence) {
        var marker = fence[1].charAt(0);
        var fenceLength = fence[1].length;
        var lang = fence[2] || '';
        var closeRe = new RegExp('^ {0,3}' + (marker === '`' ? '`' : '~') + '{' + fenceLength + ',}\\s*$');
        var buf = [];

        i += 1;
        while (i < lines.length && !closeRe.test(lines[i])) {
          buf.push(lines[i]);
          i += 1;
        }
        if (i < lines.length) i += 1; // 跳过结束围栏（流式未闭合时不存在）

        html += renderCodeBlock(buf.join('\n'), lang);
        continue;
      }

      if (!line.trim()) { i += 1; continue; }

      // 标题
      var heading = RE_HEADING.exec(line);
      if (heading) {
        var level = heading[1].length;
        var title = heading[2].replace(/\s+#+\s*$/, '');
        html += '<h' + level + '>' + renderInline(title) + '</h' + level + '>';
        i += 1;
        continue;
      }

      // 分隔线
      if (RE_HR.test(line)) {
        html += '<hr>';
        i += 1;
        continue;
      }

      // 引用（内部递归解析）
      if (RE_QUOTE.test(line)) {
        var quoted = [];
        while (i < lines.length && RE_QUOTE.test(lines[i])) {
          quoted.push(lines[i].replace(/^ {0,3}>\s?/, ''));
          i += 1;
        }
        html += '<blockquote>' + render(quoted.join('\n')) + '</blockquote>';
        continue;
      }

      // 无序列表
      if (RE_UL.test(line)) {
        var ulItems = [];
        while (i < lines.length && RE_UL.test(lines[i])) {
          ulItems.push(lines[i].replace(RE_UL, ''));
          i += 1;
        }
        html += '<ul>' + ulItems.map(function (item) {
          return '<li>' + renderInline(item) + '</li>';
        }).join('') + '</ul>';
        continue;
      }

      // 有序列表
      if (RE_OL.test(line)) {
        var olItems = [];
        while (i < lines.length && RE_OL.test(lines[i])) {
          olItems.push(lines[i].replace(RE_OL, ''));
          i += 1;
        }
        html += '<ol>' + olItems.map(function (item) {
          return '<li>' + renderInline(item) + '</li>';
        }).join('') + '</ol>';
        continue;
      }

      // 普通段落
      var paragraph = [line];
      i += 1;
      while (i < lines.length && lines[i].trim() && !isBlockStart(lines[i])) {
        paragraph.push(lines[i]);
        i += 1;
      }
      html += '<p>' + renderInline(paragraph.join('\n')).replace(/\n/g, '<br>') + '</p>';
    }

    return html;
  }

  global.Markdown = {
    render: render,
    escapeHtml: escapeHtml
  };
})(window);
