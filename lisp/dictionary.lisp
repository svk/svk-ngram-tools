#+asdf (progn
	 (asdf:operate 'asdf:load-op :gzip-stream)
	 (asdf:operate 'asdf:load-op :cl-utilities))

(defun parse-dictionary-entry (string)
  (let ((sseq (cl-utilities:split-sequence #\Tab string :count 2)))
    (values (car sseq)
	    (parse-integer (cadr sseq)))))

(defun read-dictionary-gzip (filename)
  (gzip-stream:with-open-gzip-file (stream filename :direction :input)
    (do* ((line (read-line stream nil nil) (read-line stream nil nil))
	  (ht (make-hash-table :test #'equal)))
	((null line) ht)
      (multiple-value-bind (key value)
	  (parse-dictionary-entry line)
	(setf (gethash key ht) value)))))


(defun read-dictionary-uncompressed (filename)
  (with-open-file (stream filename :direction :input)
    (do* ((line (read-line stream nil nil) (read-line stream nil nil))
	  (ht (make-hash-table :test #'equal)))
	((null line) ht)
      (multiple-value-bind (key value)
	  (parse-dictionary-entry line)
	(setf (gethash key ht) value)))))
