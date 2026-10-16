// Exporta sonidos, películas y caídas del emulador web a un header C++ (VoiceData.h)
const fs = require('fs');
const html = fs.readFileSync(process.argv[2], 'utf8').split('\n');
const grab = name => { const l = html.find(x => x.startsWith(`const ${name} = `)); return eval('(' + l.slice(`const ${name} = `.length).replace(/;\s*$/, '') + ')'); };
const FILMS = grab('FILMS'), REL = grab('REL'), VOICE_DATA = grab('VOICE_DATA');
function decCh(a) { return (a & 1) | ((a >> 2) & 1) << 1 | ((a >> 3) & 1) << 2; }
function parseVoice(txt) {   // idéntico al emulador
  const parts = [{}, {}];
  for (const p of txt.split(/\s+/)) {
    if (!p) continue;
    const [a, d] = p.split("=").map(x => parseInt(x, 16));
    const g = a >> 4;
    if (g === 7) parts[a & 7].g7 = d; else parts[decCh(a)][g * 2 + ((a >> 1) & 1)] = d;
  }
  return parts.filter(p => Object.keys(p).length).map(p => {
    const v = {}, def = { 2: 0, 3: 0x81, 4: 0x20, 5: 0x30, 6: 0xF0, 7: 0xF0, 8: 0, 9: 0, 11: 0x73, 13: 0x30 };
    for (let k = 2; k <= 13; k++) v[k] = p[k] !== undefined ? p[k] : def[k];
    if (p[10] === undefined) v[10] = v[11];
    if (p[12] === undefined) v[12] = p[10] === undefined ? v[13] : 0;
    v.g7 = p.g7 !== undefined ? p.g7 : 0xE0; v[3] &= 0x7F;
    return v;
  });
}

// ---- Galego: nomes dos sons e descricións (a interface é só en galego) ----
const NOMES_GL = ["Piano","Piano honky-tonk","Piano eléctrico","Piano de xoguete","Clavicémbalo","Celesta","Celesta de cristal","Clavinet funky","Órgano de igrexa","Órgano de tubos",
 "Harmonio","Órgano eléctrico","Órgano de jazz","Órgano con trémolo","Órgano de transistores","Organillo","Acordeón","Bandoneón","Trompeta","Trompa",
 "Trombón","Tuba","Metais sintéticos","Metais 1","Metais 2","Metais 3","Frautín","Frauta","Óboe","Fagot",
 "Clarinete","Clarinete baixo","Saxo soprano","Saxo tenor","Saxos","Trompa alpina","Frauta de Pan","Gaita","Ocarina","Violín",
 "Violonchelo","Cordas","Guitarra clásica","Guitarra acústica","Guitarra de jazz","Guitarra con trémolo","Guitarra de 12 cordas","Guitarra rock","Mandolina","Mandolinas",
 "Ukelele","Banjo","Laúde","Sitar","Guitarra hawaiana","Arpa","Shamisen","Koto","Contrabaixo","Contrabaixo con arco",
 "Baixo eléctrico","Baixo con púa","Baixo slap","Baixo sintético","Marimba","Vibráfono","Glockenspiel","Campaíñas","Carillón","Campás tubulares",
 "Tom sintético","Timbais","Steel drum","Asubío de samba","Voz humana","Coro masculino","Coro feminino","Caixa de música","Harmónica de cristal","Serra musical",
 "Bloque de xeo","Cristal","Pinga","Insecto","Sinal de teléfono","Timbre de teléfono","Golf","Moto","Automóbil","Ondas",
 "Helicóptero","Bucina","Tren","Pantasma","Cometa","Galaxia","Extraterrestre","Alarma","OVNI","Onda senoidal"];
function descGL(d, stereo) {
  const R = [
    [/Estático: el teclado solo pulsa la nota y el chip hace todo lo demás\./g, "Estático: o teclado só preme a nota e o chip fai todo o demais."],
    [/Estático, pero con dos pianos un poco desafinados entre sí\./g, "Estático, pero con dous pianos un pouco desafinados entre si."],
    [/Estático, con dos guitarras a la vez como una de 12 cuerdas\./g, "Estático, con dúas guitarras á vez coma unha de 12 cordas."],
    [/Arranca un poco desafinado y llega a la nota en (\d+) ms\./g, "Arrinca un pouco desafinado e chega á nota en $1 ms."],
    [/Trémolo a (\d+) Hz y entra a los ([\d,]+) s\./g, "Trémolo a $1 Hz que entra aos $2 s."],
    [/que entra a los ([\d,]+) s\./g, "que entra aos $1 s."],
    [/Vuelve a pulsar la nota él solo unas (\d+) veces por segundo\./g, "Repite a nota por si só unhas $1 veces por segundo."],
    [/Vuelve a pulsar la nota él solo cada ([\d,]+) s\./g, "Repite a nota por si só cada $1 s."],
    [/Cambia de volumen o de brillo con el tiempo\./g, "Cambia de volume ou de brillo co tempo."],
    [/El tono tiembla muy rápido \((\d+) Hz\): efecto, no vibrato\./g, "O ton treme moi rápido ($1 Hz): efecto, non vibrato."],
    [/La nota sube de tono de golpe al empezar, como una gota\./g, "A nota sobe de ton de golpe ao comezar, coma unha pinga."],
    [/El tono sube poco a poco \(el motor acelera\) con un temblor de (\d+) Hz\./g, "O ton sobe aos poucos (o motor acelera) cun tremor de $1 Hz."],
    [/El volumen sube y baja como olas, cada canal a su ritmo\./g, "O volume sobe e baixa coma ondas, cada canle ao seu ritmo."],
    [/Salta entre octavas unas (\d+) veces por segundo\./g, "Salta entre oitavas unhas $1 veces por segundo."],
    [/El volumen sube y baja despacio \(cada ([\d,]+) s\)\./g, "O volume sobe e baixa amodo (cada $1 s)."],
    [/Dos canales a la vez \(izquierda y derecha\)\./g, stereo ? "Dúas canles á vez: unha á esquerda e outra á dereita." : "Dúas canles á vez, superpostas no centro."],
  ];
  for (const [a, b] of R) d = d.replace(a, b);
  if (/[ ]la |[ ]los |[ ]una |[ ]y |[ ]el teclado/.test(' ' + d)) console.error('SEN TRADUCIR?', d);
  return d;
}

const esc = s => JSON.stringify(s).replace(/[\u0080-￿]/g, c => Buffer.from(c).toString('hex').match(/../g).map(h => '\\x' + h).join('') + '""');
let ev = [], out = [];
out.push('// Generado por tools/export_data.js a partir del emulador web. No editar a mano.');
out.push('#pragma once\n#include <cstdint>\nnamespace ym {');
out.push('struct Part { uint8_t r[16]; uint8_t g7; };   // r[k], k = grupo*2 + operador (2..13)');
out.push('struct FilmEv { uint32_t t; uint8_t part, k, d; };   // t en ms desde la pulsación; k = 14 → registro de canal');
out.push('struct Voice { const char* name; int nParts; Part parts[2]; int8_t rel[2][2]; uint32_t ev0, evN; uint32_t end; int16_t init[2][16]; const char* desc; };   // init: registros que la película fija antes de pulsar (-1 = no; [14] = canal)');
const voices = [];
VOICE_DATA.forEach(([name, txt], i) => {
  const parts = parseVoice(txt), F = FILMS[i] || {}, R = REL[i] || [];
  const ev0 = ev.length; let t = 0;
  if (F.e) for (const tok of F.e.split(' ')) {
    if (tok[0] === '+') { t += +tok.slice(1); continue; }
    const v = parseInt(tok, 16); ev.push([Math.round(t), v >> 16, (v >> 8) & 255, v & 255]);
  }
  const evN = ev.length - ev0, end = evN ? ev[ev.length - 1][0] : 0;
  const P = [0, 1].map(pi => { const p = parts[pi]; if (!p) return '{{0},0}'; const r = []; for (let k = 0; k < 16; k++) r.push(p[k] ?? 0); return `{{${r.join(',')}},${p.g7}}`; });
  const rel = [0, 1].map(pi => { const r = R[pi] || [4, 6]; return `{${r[0]},${r[1]}}`; });
  const init = [0, 1].map(pi => { const o = (F.i && F.i[pi]) || {}; const r = []; for (let k = 0; k < 16; k++) { const kk = k === 14 ? "g7" : k; r.push(o[kk] !== undefined ? o[kk] : -1); }; return `{${r.join(',')}}`; });
  const g7a = parts[0] ? parts[0].g7 : 0, g7b = parts[1] ? parts[1].g7 : 0;
  const stereo = parts.length > 1 && ((g7a >> 5) & 3) !== 3 && ((g7b >> 5) & 3) !== 3 && ((g7a >> 5) & 3) !== ((g7b >> 5) & 3);
  voices.push(`  { ${esc(NOMES_GL[i] || name || '')}, ${parts.length}, {${P.join(',')}}, {${rel.join(',')}}, ${ev0}, ${evN}, ${end}, {${init.join(',')}}, ${esc(descGL(F.d || '', stereo))} }`);
});
out.push(`static const FilmEv FILM_EVENTS[${Math.max(1, ev.length)}] = {`);
for (let i = 0; i < ev.length; i += 8) out.push('  ' + ev.slice(i, i + 8).map(e => `{${e.join(',')}}`).join(',') + ',');
out.push('};');
out.push(`static const Voice VOICES[${voices.length}] = {\n${voices.join(',\n')}\n};`);
out.push(`constexpr int NUM_VOICES = ${voices.length};\n}`);
fs.writeFileSync(process.argv[3], out.join('\n') + '\n');
// comprobaciones
const keysI = new Set(); FILMS.forEach(F => F.i && Object.values(F.i).forEach(o => Object.keys(o).forEach(k => keysI.add(k))));
const ks = new Set(ev.map(e => e[2]));
console.log('voces', voices.length, 'eventos', ev.length, 'claves init', [...keysI].join(','), 'k en eventos', [...ks].sort((a,b)=>a-b).join(','),
  'partes>1 en ev', ev.filter(e=>e[1]>1).length, 'REL negativos', JSON.stringify(REL).match(/-1/g)?.length, 'max end', Math.max(...ev.map(e=>e[0])));
console.log('tiempos no enteros', FILMS.filter(F=>F.e && /\+\d+\.\d/.test(F.e)).length);
