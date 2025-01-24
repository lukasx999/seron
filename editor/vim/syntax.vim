syn keyword spxKeyword
      \ if
      \ else
      \ while
      \ val
      \ proc
      \ asm
      \ extern
      \ define
      \ return
      \ macro

syn match spxNumber "\d+"
syn match spxOperator "[,.=+-\*/;(){}'`]"

syn region spxString start=/"/ end=/"/
syn match spxComment "#.*$"
syn region spxMultilineComment start=/##/ end=/##/


" TODO:
" syn region spxAsm start=/asm\s*\zs{/ end=/}/ contains=spxAsmKw
" syn region spxAsm start=/{/ end=/}/ containedin=spxAsmKw
" highlight default link spxAsm String


highlight default link spxKeyword Keyword
highlight default link spxString String
highlight default link spxNumber Number
highlight default link spxOperator Operator
highlight default link spxComment Comment
highlight default link spxMultilineComment Comment
