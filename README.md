COMPLAC: um compilador para Linguagem SLAC²
O projeto consiste na implementação, em C, de um compilador que atenda por completo a especificação oficial da linguagem SLAC². O compilador COMPLAC deverá ler o código fonte, reconhecer os elementos da linguagem, validar sua estrutura e produzir o código as instruções para uma máquina virtual.

O desenvolvimento será organizado em duas fases. Nesta primeira etapa, o objetivo é cumprir as tarefas cabíveis as análises léxica e sintática, e construir tabela de símbolos, de modo a estabelecer a infraestrutura necessária para a análise semântica e para a geração de código, que ocorrerão na fase seguinte.

 

1.    Análise Léxica, Sintática e Tabela de Símbolos
Nesta primeira parte do projeto faremos a implementação da frente de análise do compilador, integrando o reconhecimento de tokens, validação sintática e registro dos identificadores encontrados.

A descrição da linguagem SLAC², incluindo informações sobre operadores, delimitadores, comandos, palavras‑reservadas e estrutura geral de um programa, encontra-se detalhada no documento de especificação.

O analisador léxico deve consumir o arquivo fonte sequencialmente, sem pré processamento. Cada chamada devolve um token contendo a categoria, o lexema e a localização (linha) em que ocorre no arquivo fonte. Deve reconhecer todos os elementos significativos da linguagem, diferenciando adequadamente literais, identificadores, delimitadores, operadores e comentários (de linha e de bloco). A resolução entre operadores unários e binários será determinada pelo contexto.

O analisador sintático deve ser implementado como um Analisador Sintático Descendente Recursivo (ASDR) que segue diretamente a gramática oficial da SLAC². Cada não terminal deve possuir função própria, e a hierarquia de expressões deve respeitar a precedência definida na especificação.

A deteção de erros deve interromper o processamento e apresentar uma mensagem informativa clara, reportando o token esperado, o token encontrado e a localização no arquivo fonte.

Por fim, a Tabela de Símbolos (TS) deve registrar o lexema, a categoria, o tipo de dado e escopo de cada identificador, contemplando variáveis globais, locais, parâmetros e sub rotinas. Adicionalmente, consideraremos nesse momento um atributo extra, usado para guardar o tamanho, no caso de vetores; ou quantidade de parâmetros, em sub-rotinas. A implementação deve refletir a estrutura da SLAC², mantendo visibilidade e resolução de nomes conforme a linguagem.

 

2.    Orientações de Implementação
A implementação será feita integralmente em C, sem uso de ferramentas automáticas de análise. Os módulos devem estabelecer funções e interfaces claramente definidas, evitando dependências ocultas e garantindo rastreabilidade do fluxo de análise.

A comunicação entre módulos ocorre apenas via cabeçalhos públicos (.h), através de funções, parâmetros e retornos que estabelecem a interface para troca de dados. Estados internos e variáveis globais não devem ser compartilhados implicitamente.

O programa de ser capaz de receber e interpretar parâmetros de linha de comando (CLI), para:

·       selecionar o arquivo‑fonte;

·       ativar geração de logs (lista de tokens, tabela de símbolos ou rastreamento);

·       definir o comportamento de diagnóstico.

 

3.    Estrutura de módulos
Recomenda-se a organização do código nos seguintes módulos (estrutura sugerida, não obrigatória):

main

Orquestração e CLI

Coordena a execução: processa parâmetros, abre arquivos, inicializa lex, parser, symtab e diag, aciona o processo de análise e, ao final, encerra todos os módulos de forma ordenada. Não realiza análise direta.

lex

Analisador Léxico

Varredura sequencial do código‑fonte, devolvendo um token por chamada. Mantém posição exata no arquivo e distingue corretamente todas as categorias lexicais da SLAC².

Interface: lex_next

parser

ASDR estrito

Consome tokens de lex e implementa a gramática da SLAC². Cada não‑terminal corresponde a uma função específica. É o responsável por criar e encerrar escopos durante a análise.

Interface: parse_program

symtab

Tabela de Símbolos com escopo

Gerencia escopos, inserção e busca de identificadores. Deve espelhar a estrutura de blocos da SLAC² e suas regras de visibilidade.

Interface: ts_insert, ts_lookup

diag

Diagnósticos e relatórios

Centraliza mensagens de erro e rastreamento, com formato padronizando das saídas. Atua como API de alto nível para lex e parser.

Interface: diag_error, diag_info

opt

Opções de execução

Interpreta a linha de comando (CLI) e disponibiliza as opções para todo o compilador.

Interface: opts_parse, opts_get

log

Registro de artefatos intermediários

Cria arquivos auxiliares opcionais contendo:

(1) lista de tokens;

(2) tabela de símbolos, com formato fixo e ordenação definida, ou

(3) rastreamento da análise sintática

 

Integração entre módulos

A integração é estritamente síncrona e unidirecional, guiada pelo main:

1.     Inicialização e opções.

O main processa a linha de comando via opt e inicializa os módulos.

2.     Ciclo de análise sintática com consumo léxico sob demanda.

O parser consume os tokens do lex, invocando lex_next conforme a expansão das produções.

3.     Criação e manutenção de escopos.

O parser controla os escopos enquanto processa regiões globals, cabeçalhos de sub-rotinas, locals ou em blocos start...end. Assim, as inserções e buscas a identificadores na TS ocorrem durante o reconhecimento das produções relacionadas.

4.     Diagnóstico centralizado e interrupção em falhas.

lex e parser não imprimem mensagens diretamente, chamam funções de diag. Erros léxicos ou sintáticos interrompem o processo.

5.     Geração de logs (opcional).

Se habilitado, via opt, são criados arquivos de logs para registrar:

·       Lista de tokens: que registra os tokens retornados pelo lex, gravando-os no formato:

linha  <CATEGORIA>  "<lexema>".

·       Tabela de símbolos: guarda o conteúdo consolidado da TS, em ordem de escopo (global → locais por subrotina), preservando a ordem de inserção e a visibilidade.

6.     Encerramento ordenado.

Concluída a análise (com sucesso ou erro), o main finaliza os módulos, fecha os arquivos e encerra a execução.

 

Parâmetros de linha de comando e formatos de saída

O binário deve aceitar chamadas na linha de comando, na forma:

complac <arquivo.slac> [--tokens | --symtab | --trace]

onde:

<arquivo.slac>: representa o path do arquivo fonte a ser compilado.

--tokens: parâmetro que ativa a criação de um arquivo de log, com o mesmo nome do fonte e extensão tk, contendo a lista dos tokens identificados pelo analisador léxico.

O registro dos tokens segue a forma: <L>  <CAT>  "<LEX>", onde <CAT> é exatamente o nome da categoria do token segundo a especificação (por ex.: sIDENTIF, sCTEINT, sIF, sATRIB, etc.)

--symtab: gera um arquivo com extensão ts, contendo a tabela de símbolos consolidada. As informações são gravadas por escopo, segue o formato:

SCOPE=<descr>  id="<lexema>"  cat=<categ>  tipo=<tipo>  extra=<atrib>

onde o campo SCOPE deve refletir o caminho de escopos (Ex: global, fn:SOMA.locals, proc:main.block#2), em conformidade com a organização de globals, locals e blocos start...end da SLAC².

--trace: cria um arquivo log com a extensão trc e registra as informações relativas ao progresso da análise, que são apresentadas por diag_info (por ex.: entrada/saída de não‑terminais).

 

4.    Artefatos para a entrega:
A entrega deve incluir todos os arquivos necessários para compilar e executar a solução em ambiente Linux utilizando o gcc, permitindo a compilação e testes via linha de comando.

A submissão deve conter:

Código‑fonte completo, incluindo todos os arquivos .c e .h que dos módulos descritos e quaisquer outros auxiliares que sejam necessários ao projeto.
As funções públicas devem ser declaradas nos cabeçalhos correspondentes, e nenhum módulo deve depender implicitamente de detalhes internos de outro.
Makefile funcional, capaz de compilar todo o projeto com um único comando (make), produzindo o binário final do compilador. O Makefile deverá utilizar obrigatoriamente as seguintes flags de compilação: -Wall -Wextra -std=c99
O conteúdo e formado dos arquivos de log deverá refletir os elementos registrados durante a análise, tais como: categorias de tokens, valores de lexemas, identificadores e suas informações. Embora esses arquivos não sejam obrigatórios na submissão, o binário deverá ser capaz de gera-los durante os testes avaliativos.
Relatório simples (opcional), na forma de readme.md, contendo quaisquer instruções específicas necessárias para compilar e executar o projeto, bem como eventuais particularidades relevantes da implementação. O relatório não substitui documentação interna do código, que deve estar presente através de comentários claros e precisos.
Os arquivos devem estar organizados em uma estrutura coerente, com nomes consistentes, sem arquivos temporários, binários ou diretórios irrelevantes.

 

5.    Critérios de avaliação
A avaliação considerará a corretude, conformidade à especificação e a clareza técnica do código desenvolvido. O desempenho do compilador será analisado a partir de programas escritos em SLAC², que incluem casos de testes válidos e outro contendo erros intencionais que avaliarão diferentes partes.

A especificação oficial da linguagem, especialmente gramática, categorias lexicais, regras de blocos e hierarquia de operadores, constitui a referência absoluta para validação.

A nota será atribuída conforme os seguintes critérios:

Atendimento aos requisitos da especificação: o compilador deve reconhecer corretamente todos os tokens, estruturas sintáticas e blocos previstos na linguagem SLAC², seguindo rigorosamente sua especificação e regras formais.
Corretude da análise léxica: o analisador léxico deve identificar e classificar adequadamente os tokens da linguagem, distinguindo corretamente operadores, literais, palavras reservadas, delimitadores e comentários, sendo capaz ainda de identificar erros lexicais e reporta-los com mensagens apropriadas
Corretude da análise sintática: o parser deve refletir fielmente a gramática da SLAC², reconhecendo estruturas aninhadas, expressões e precedências, blocos, sub-rotinas e outros, conforme especificação. Erros sintáticos devem ser identificados e relatados de forma clara e bem construida.
Construção e manutenção da tabela de símbolos: a TS deve registrar corretamente escopos globais, locais e blocos internos; controlar inserção e busca; identificar declarações duplicadas; e distinguir categorias e tipos exatamente como previsto na linguagem.
O projeto pode ser feito individualmente ou em duplas, sendo submetidos por apenas um dos integrantes quando em grupo.

Deverão ser observadas as orientações sobre "Boas práticas no desenvolvimento de trabalhos acadêmicos". Lembrando que casos de plágio estão passíveis da aplicação de sanções acadêmicas, conforme determinado pelas normativas da FCI.