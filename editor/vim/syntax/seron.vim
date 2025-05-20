syn keyword srnKeyword
      \ if
      \ else
      \ elsif
      \ while
      \ let
      \ proc
      \ asm
      \ extern
      \ define
      \ return
      \ macro
      \ char
      \ int
      \ long
      \ void
      \ table

syn match srnOperator "[,.=+-\*/;(){}'`]"

syn region srnString start=/"/ end=/"/
syn region srnChar start=/'/ end=/'/
syn match srnComment "#.*$"
syn region srnMultilineComment start=/##/ end=/##/

highlight default link srnKeyword Keyword
highlight default link srnString String
highlight default link srnChar String
highlight default link srnNumber Number
highlight default link srnOperator Operator
highlight default link srnComment Comment
highlight default link srnMultilineComment Comment
