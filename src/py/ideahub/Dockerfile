FROM fasionchan/netbox:0.8

RUN apt install sqlite3 -y
RUN apt --no-install-recommends install python3-pip -y

ADD requirements.txt /root/ideahub/
RUN pip install -r /root/ideahub/requirements.txt

ADD templates/ /root/ideahub/templates
ADD ideahub.py /root/ideahub/
ADD ideahub.sql /root/ideahub/
ADD resetdb.sh /root/ideahub/
ADD run.sh /root/ideahub/

RUN /root/ideahub/resetdb.sh

EXPOSE 5000

CMD ["/root/ideahub/run.sh"]
