(asdf:operate 'asdf:load-op :cl-ppcre)
(asdf:operate 'asdf:load-op :gzip-stream)
(asdf:operate 'asdf:load-op :cl-utilities)

(defpackage #:steinavk-master
  (:use :cl
        :asdf
        :cl-ppcre
	:gzip-stream)
  (:import-from :cl-utilities split-sequence))

