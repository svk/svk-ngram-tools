(in-package :steinavk-master)


(defparameter *standard-input-raw-vocabulary*
    #P"~/svk-ngram-tools/data/vocab")

(defparameter *standard-output-dictionary*
    #P"~/svk-ngram-tools/data/working/vocabulary")

(defparameter *standard-output-transformer*
    #P"~/svk-ngram-tools/data/working/transformer")

(defparameter *top-level-domains*
    '("com" "org" "net" "museum" "fr" "de" "uk" "za" "ca" "us" "nz" "no" "se" "dk"))

(defun intersperse (elt list)
  (cond ((or (null list)
	     (null (cdr list)))
	 list)
	(t
	 (cons (car list)
	       (cons elt
		     (intersperse elt (cdr list)))))))

(defun string-join (joiner list)
  (apply #'concatenate
	 (cons 'string
	       (intersperse joiner list))))

(defparameter *re-digit* "[0-9]")
(defparameter *re-email* "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$")
(defparameter *re-full-uri* "^[a-zA-Z]{3,4}://[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}")
(defparameter *re-top-level-uri* "^[a-zA-Z]{3,4}://[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}/?$")
(defparameter *re-domain-name* (concatenate
				   'string
				 "^(?:[a-zA-Z0-9-]+\\.)+"
				 "("
				 (string-join "|" *top-level-domains*)
				 ")"
				 "$"))
(defparameter *re-unusual-domain-name* "^(www|ftp)\.(?:[a-zA-Z0-9-]+\\.)+[a-zA-Z]{2,4}")
(defparameter *re-ip-address* "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")
(defparameter *re-tagged-hex* "^(0x[0-9a-fA-F]+|0[0-9a-fA-F]+h)") ;; TODO fix but allow for compat, needs $
(defparameter *re-small-number* "^[0-9]{1,2}$")

;; Matched after substitution of all digits with 0
(defparameter *re-number-range* "0+-0+")

(defun transform-token (token-string)
  (cond ((scan *re-tagged-hex* token-string)
	 "<HEX_NUMBER>")
	((scan *re-ip-address* token-string)
	 "<IP_ADDRESS>")
	((or (scan *re-domain-name* token-string)
	     (scan *re-unusual-domain-name* token-string))
	 "<DOMAIN_NAME>")
	((scan *re-top-level-uri* token-string)
	 "<TOP_URI>")
	((scan *re-full-uri* token-string)
	 "<URI>")
	((scan *re-email* token-string)
	 "<EMAIL>")
	((scan *re-small-number* token-string) ;; Good reason for this? reconsider
	 token-string) 
	(t
	 (let* ((digits-substituted (regex-replace-all *re-digit* token-string "0")))
	   digits-substituted))))

(defun for-tokens-in-vocabulary (raw-vocabulary-filename f &optional (progress nil))
  (with-open-file (stream raw-vocabulary-filename :direction :input)
    (do* ((line (read-line stream nil nil) (read-line stream nil nil)))
	((null line) nil)
      (let* ((key-value (split-sequence #\tab line :count 2))
	     (token (car key-value)))
	(funcall f token)
	(when progress
	  (funcall progress))))))

(defun create-dictionaries (raw-vocabulary-filename f dictionary-filename transformer-filename)
  (with-open-file (out-dict dictionary-filename :direction :output :if-exists :supersede)
    (with-open-file (out-trans transformer-filename :direction :output :if-exists :supersede)
      (let ((token-numbers (make-hash-table :test #'equal))
	    (processed-written-to-transformer (make-hash-table :test #'equal))
	    (unprocessed-written-to-transformer (make-hash-table :test #'equal))
	    (next-number -1)
	    (processed 0))
	(labels ((write-key-value (stream token number)
		   (write-string token stream)
		   (write-char #\tab stream)
		   (write-string (write-to-string number) stream)
		   (write-char #\newline stream)
		   number)
		 (write-unprocessed (token number)
		   (when (gethash token processed-written-to-transformer)
		     (error (format nil "collision: ~a maps to self" token)))
		   (write-key-value out-trans token number)
		   (setf (gethash token unprocessed-written-to-transformer) number))
		 (write-processed (token number)
		   (when (gethash token unprocessed-written-to-transformer)
		     (error (format nil "collision: ~a maps to self" token)))
		   (write-key-value out-trans token number)
		   (write-key-value out-dict token number)
		   (setf (gethash token processed-written-to-transformer) number))
		 (handle-transformation (unprocessed-token processed-token)
		   (let ((prior-n (gethash processed-token token-numbers nil)))
		     (if prior-n
			 (write-unprocessed unprocessed-token prior-n)
		       (let ((new-n (setf (gethash processed-token token-numbers) (incf next-number))))
			 (unless (or (null unprocessed-token)
				     (equal unprocessed-token processed-token))
			   (write-unprocessed unprocessed-token new-n))
			 (write-processed processed-token new-n)))))
		 (handle-artificial-token (token)
		   (handle-transformation nil token))
		 (handle-token (token)
		   (handle-transformation token (funcall f token)))
		 (handle-progress ()
		   (when (zerop (mod (incf processed) 100000))
		     (format t "[~a tokens processed, ~a tokens produced]~%" processed (1+ next-number)))))
	  (handle-artificial-token "<*>")
	  (handle-artificial-token "<PP_UNK>")
	  (for-tokens-in-vocabulary raw-vocabulary-filename #'handle-token #'handle-progress))))))

(defun create-standard-dictionaries ()
  (create-dictionaries *standard-input-raw-vocabulary*
		       #'transform-token
		       *standard-output-dictionary*
		       *standard-output-transformer*))

				    
				  
				 
	  
	
	     