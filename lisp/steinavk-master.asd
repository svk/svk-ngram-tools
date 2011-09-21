(defpackage #:steinavk-master
  (:use :cl
	:asdf
	:cl-ppcre-unicode))

(in-package :steinavk-master)

(defsystem steinavk-master
    :name "steinavk's master project (Lisp components)"
    :author "Steinar Vittersø Kaldager"
    :components ((:file "preprocessing")))

    