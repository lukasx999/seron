syn keyword srnKeyword
      \ if
      \ else
      \ elsif
      \ while
      \ val
      \ proc
      \ asm
      \ extern
      \ define
      \ return
      \ macro
      \ byte
      \ int
      \ size
      \ void

syn match srnOperator "[,.=+-\*/;(){}'`]"

syn region srnString start=/"/ end=/"/
syn match srnComment "#.*$"
syn region srnMultilineComment start=/##/ end=/##/

highlight default link srnKeyword Keyword
highlight default link srnString String
highlight default link srnNumber Number
highlight default link srnOperator Operator
highlight default link srnComment Comment
highlight default link srnMultilineComment Comment
