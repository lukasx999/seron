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
      \ byte
      \ int
      \ size

" syn match spxNumber "\d+"
syn match spxOperator "[,.=+-\*/;(){}'`]"

syn region spxString start=/"/ end=/"/
syn match spxComment "#.*$"
syn region spxMultilineComment start=/##/ end=/##/

highlight default link spxKeyword Keyword
highlight default link spxString String
highlight default link spxNumber Number
highlight default link spxOperator Operator
highlight default link spxComment Comment
highlight default link spxMultilineComment Comment
