syntax keyword spxKeyword
      \ if
      \ while
      \ val
      \ proc
      \ asm

syntax match spxNumber "\d+"
syntax match spxOperator "[,.=+-\*/;(){}]"

syntax region spxString start=/"/ end=/"/
syntax match spxComment "#.*$"
syntax region spxMultilineComment start=/##/ end=/##/

highlight default link spxKeyword Keyword
highlight default link spxString String
highlight default link spxNumber Number
highlight default link spxOperator Operator
highlight default link spxComment Comment
highlight default link spxMultilineComment Comment
